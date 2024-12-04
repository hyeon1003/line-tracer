# line-tracer

## 라인검출 코드

### 전처리 함수 
'''void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh) {
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
}'''

### 라인검출함수

'''void findObjects(const Mat& thresh, Point& tmp_pt, Mat& result, Mat& stats, Mat& centroids) {
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
'''

### 라인시각화 함수

'''void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result) {
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
}'''

### 에러 구하는 함수
'''int getError(const Mat& frame, const Point& po) {
    return (frame.cols / 2 - po.x);
}'''
