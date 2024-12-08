# line-tracer

## 라인검출 코드

### 전처리 함수 



```
void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh) {
    source >> frame;
    if (frame.empty()) {
        cerr << "empty frame" << endl;
        return;
    }

    // 그레이스케일 변환 및 밝기 보정
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    Scalar bright_avg = mean(gray);
    gray = gray + (100 - bright_avg[0]);

    // 이진화 (Otsu 방법)
    threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 관심 영역(ROI) 선택 (하단 1/4)
    int r_pts = thresh.rows / 4 * 3;
    Rect r(0, r_pts, thresh.cols, thresh.rows - r_pts);
    thresh = thresh(r);
}
```

frame= 원본영상
gray=그레이스케일영상(밝기 보정을 통해 100으로 조정)
thresh=이진화 영상 otsu알고리즘 사용
관심영역 설정(영상 하단부 1/4 영역 )

### 라인검출함수


```
void findObjects(const Mat& thresh, Point& tmp_pt, Mat& result, Mat& stats, Mat& centroids) {
    Mat labels;
    int cnt = connectedComponentsWithStats(thresh, labels, stats, centroids);

    // 초기값 설정
    result = thresh.clone();
    cvtColor(result, result, COLOR_GRAY2BGR);

    int min_index = -1;
    int min_dist = thresh.cols;

    for (int i = 1; i < cnt; i++) {
        int area = stats.at<int>(i, 4);  // 객체 면적
        if (area > 100) {  // 노이즈 제거
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));
            int dist = norm(Point(x, y) - tmp_pt);
            if (dist < min_dist && dist <= 75) {  // 거리 조건
                min_dist = dist;
                min_index = i;
            }
        }
    }

    // 중심 업데이트
    if (min_index != -1 && min_dist <= 75) {
        tmp_pt = Point(cvRound(centroids.at<double>(min_index, 0)),
                       cvRound(centroids.at<double>(min_index, 1)));
    }
}
```

이진화된 영상을 입력으로 받아 connectedComponentsWithStats 함수를 통해 객체를 찾음
이진화된 영상을 복제하여 컬러로 변환하여 시각화함
최소거리 설정 최초에는 -1로설정하여 유효한 객체가 없음을 나타냄
min_index,min_dist는 가장 가까운 객체를 선택하기 위해 최소 거리와 해당 인덱스를 저장
반복문을 통해 객체를 순회하여 면적,중심 좌표를 이용해 최소 거리와 인덱스를 업데이트
norm함수는 opencv함수로 두 점 사이의 거리를 계산하는 함수
라인검출 알고리즘: 현재 계산한거리가 이전에 저장된 거리 보다 작고 거리가 75이하인 경우에만 최소 거리와 인덱스를 업데이트함
이전 중심점과 멀리 떨어진 객체는 추적중인 라인이 아닐 가능성이 높아 75로 설정함
면적 기준이 100인 이유는 작은 면적의 객체는 불필요한 객체일 가능성이 커서 필터링함



### 라인시각화 함수


```
void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result) {
    bool isTracked = false; // 현재 추적 중인 라인이 있는지 확인
    for (int i = 1; i < stats.rows; i++) {
        int area = stats.at<int>(i, 4); // 객체 면적
        if (area > 100) { // 충분히 큰 객체만 처리
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));

            // 추적 중인 라인인지 확인
            if (x == tmp_pt.x && y == tmp_pt.y) {
                isTracked = true; // 추적 중인 라인이 있음
                // 추적 중인 라인은 빨간색 표시
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(0, 0, 255)); // 빨간 박스
                circle(result, Point(x, y), 5, Scalar(0, 0, 255), -1); // 빨간 점
            }
            else {
                // 추적 중이 아닌 후보 라인은 파란색 표시
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(255, 0, 0)); // 파란 박스
                circle(result, Point(x, y), 5, Scalar(255, 0, 0), -1); // 파란 점
            }
        }
    }
    // 라인이 사라졌을 때 이전 중심 유지
    if (!isTracked) {
        circle(result, tmp_pt, 5, Scalar(0, 0, 255), -1); // 빨간 점 표시
    }
}
```

추적중인 라인을 시각화 하는 함수

isTracked=추적중인 라인이 있는지 나타내는 플래그( 라인이 사라져도 표시하기 위해)
반복문을 통해 각 객체를 시각화함 여기서 면적이 100이상인 이유는 100미만은 노이즈일 확률이 크기때문이다.

현재 객체의 중심 좌표가 이전 중심 좌표와 동일하면 추적중인 객체로 간주하고 빨간색으로 표시
현재 객체가 추적중인 라인이 아니면 후보라인으로 간주하여 파란색으로 표시

추적중인 라인이 없어질 경우 이전 중심좌표를 기억해 빨간점으로 표시하여 다시 라인이 나타날때의 이 값을 통해 계속 추적이 가능하게끔 함


### 에러 구하는 함수


```
int getError(const Mat& frame, const Point& po) {
    return (frame.cols / 2 - po.x);
}
```
