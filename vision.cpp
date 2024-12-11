/*#include "vision.hpp"

void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh) {
    source >> frame;
    if (frame.empty()) {
        cerr << "empty frame" << endl;
        return;
    }

    // 그레이스케일 변환
    cvtColor(frame, gray, COLOR_BGR2GRAY);

    // 밝기 보정
    Scalar bright_avg = mean(gray); // 현재 평균 밝기 계산
    gray = gray + (100 - bright_avg[0]); // 밝기 조정

    // 이진화
    threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);
    // 관심 영역(ROI) 자르기: 영상의 하단 1/4만 남김
    int r_pts = thresh.rows / 4 * 3;
    Rect r(0, r_pts, thresh.cols, thresh.rows - r_pts);
    thresh = thresh(r);
}

void findObjects(const Mat& thresh, Point& tmp_pt, Mat& result, Mat& stats, Mat& centroids) {
    Mat labels;
    int cnt = connectedComponentsWithStats(thresh, labels, stats, centroids);
    result = thresh.clone();
    cvtColor(result, result, COLOR_GRAY2BGR);

    int min_index = -1;
    int min_dist = result.cols; // 최소 거리 저장

    for (int i = 1; i < cnt; i++) {
        int area = stats.at<int>(i, 4); // 객체 면적 확인

        if (area > 100) { // 면적이 충분히 큰 객체만 처리
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));
            int dist = norm(Point(x, y) - tmp_pt); // 거리 계산

            if (dist < min_dist && dist <= 75) { // 100 → 75로 수정
                min_dist = dist;
                min_index = i;
            }

        }
    }
    if (min_index != -1 && min_dist <= 75) {
        tmp_pt = Point(cvRound(centroids.at<double>(min_index, 0)),
                       cvRound(centroids.at<double>(min_index, 1)));
    }
}

void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result) {
    for (int i = 1; i < stats.rows; i++) {
        int area = stats.at<int>(i, 4);
        if (area > 100) {
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));
            if (x == tmp_pt.x) {
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(0, 0, 255)); // 빨간색
                circle(result, Point(x, y), 5, Scalar(0, 0, 255), -1); // 중심점
            } else {
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(255, 0, 0)); // 파란색
                circle(result, Point(x, y), 5, Scalar(255, 0, 0), -1); // 중심점
            }
        }
    }
}

double getError(const Mat& frame, const Point& po, double gain) {
    return (frame.cols / 2 - po.x) * gain;
}*/

#include "vision.hpp"

// 입력 프레임 전처리 함수
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

// 라인 검출 및 중심 업데이트
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
        if (area > 100) {  // 노이즈 제거(작은 객체 무시) 기준 100
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

// 라인 시각화
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


// 에러 계산
int getError(const Mat& frame, const Point& po) {
    return (frame.cols / 2 - po.x);
}

void controlDynamixel(Dxl &dxl, bool motor_active, double error, double gain, int &vel1, int &vel2) {
    const int MAX_ERROR = 150;  // 최대 error 값 제한
    error = (error > MAX_ERROR) ? MAX_ERROR : ((error < -MAX_ERROR) ? -MAX_ERROR : error);

    if (motor_active) {
        if (abs(error) > 200) {
            // 급격한 회전 동작
            vel1 = (error > 0) ? 50 : -50;
            vel2 = (error > 0) ? -50 : 50;
        } else {
            // 일반적인 직진 및 회전 동작
            vel1 = 100 - gain * error;
            vel2 = -(100 + gain * error);
        }
        limitVelocity(vel1, vel2);  // 역방향 회전 방지
        dxl.setVelocity(vel1, vel2);
    } else {
        vel1 = vel2 = 0;  // 모터 정지
        dxl.setVelocity(vel1, vel2);
    }
}

void limitVelocity(int &vel1, int &vel2) {// 속도 제한 함수
    if (vel1 < 0) vel1 = 0;
    if (vel2 > 0) vel2 = 0;
}
