#include "vision.hpp"

// 입력 프레임 전처리 함수
void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh) {
    source >> frame;
    if (frame.empty()) {
        cerr << "Empty frame!" << endl;
        return;
    }

    // 그레이스케일 변환 및 밝기 보정
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    Scalar bright_avg = mean(gray);
    gray = gray + (100 - bright_avg[0]);

    // 이진화 (Otsu 방법)
    threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 관심 영역(ROI) 설정 (하단 1/4만 추출)
    int roi_start = thresh.rows / 4 * 3;
    Rect roi(0, roi_start, thresh.cols, thresh.rows - roi_start);
    thresh = thresh(roi);
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
        int area = stats.at<int>(i, 4); // 객체 면적
        if (area > 100) { // 노이즈 제거
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));
            int dist = norm(Point(x, y) - tmp_pt);
            if (dist < min_dist && dist <= 75) {
                min_dist = dist;
                min_index = i;
            }
        }
    }

    // 가장 가까운 객체의 중심 업데이트
    if (min_index != -1 && min_dist <= 75) {
        tmp_pt = Point(cvRound(centroids.at<double>(min_index, 0)),
                       cvRound(centroids.at<double>(min_index, 1)));
    }
}

// 라인 시각화 함수
void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result) {
    bool isTracked = false;
    for (int i = 1; i < stats.rows; i++) {
        int area = stats.at<int>(i, 4);
        if (area > 100) {
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));

            if (x == tmp_pt.x && y == tmp_pt.y) {
                isTracked = true;
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(0, 0, 255)); // 빨간색 박스
                circle(result, Point(x, y), 5, Scalar(0, 0, 255), -1); // 빨간 점
            } else {
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(255, 0, 0)); // 파란색 박스
                circle(result, Point(x, y), 5, Scalar(255, 0, 0), -1); // 파란 점
            }
        }
    }
    if (!isTracked) {
        circle(result, tmp_pt, 5, Scalar(0, 0, 255), -1); // 빨간 점 유지
    }
}

// 에러 계산 함수
int getError(const Mat& frame, const Point& po) {
    return (frame.cols / 2 - po.x);
}

// Dynamixel 제어 함수
#include "vision.hpp"

// 입력 프레임 전처리 함수
void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh) {
    source >> frame;
    if (frame.empty()) {
        cerr << "Empty frame!" << endl;
        return;
    }

    // 그레이스케일 변환 및 밝기 보정
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    Scalar bright_avg = mean(gray);
    gray = gray + (100 - bright_avg[0]);

    // 이진화 (Otsu 방법)
    threshold(gray, thresh, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 관심 영역(ROI) 설정 (하단 1/4만 추출)
    int roi_start = thresh.rows / 4 * 3;
    Rect roi(0, roi_start, thresh.cols, thresh.rows - roi_start);
    thresh = thresh(roi);
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
        int area = stats.at<int>(i, 4); // 객체 면적
        if (area > 100) { // 노이즈 제거
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));
            int dist = norm(Point(x, y) - tmp_pt);
            if (dist < min_dist && dist <= 75) {
                min_dist = dist;
                min_index = i;
            }
        }
    }

    // 가장 가까운 객체의 중심 업데이트
    if (min_index != -1 && min_dist <= 75) {
        tmp_pt = Point(cvRound(centroids.at<double>(min_index, 0)),
                       cvRound(centroids.at<double>(min_index, 1)));
    }
}

// 라인 시각화 함수
void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result) {
    bool isTracked = false;
    for (int i = 1; i < stats.rows; i++) {
        int area = stats.at<int>(i, 4);
        if (area > 100) {
            int x = cvRound(centroids.at<double>(i, 0));
            int y = cvRound(centroids.at<double>(i, 1));

            if (x == tmp_pt.x && y == tmp_pt.y) {
                isTracked = true;
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(0, 0, 255)); // 빨간색 박스
                circle(result, Point(x, y), 5, Scalar(0, 0, 255), -1); // 빨간 점
            } else {
                rectangle(result, Rect(stats.at<int>(i, 0), stats.at<int>(i, 1),
                                       stats.at<int>(i, 2), stats.at<int>(i, 3)),
                          Scalar(255, 0, 0)); // 파란색 박스
                circle(result, Point(x, y), 5, Scalar(255, 0, 0), -1); // 파란 점
            }
        }
    }
    if (!isTracked) {
        circle(result, tmp_pt, 5, Scalar(0, 0, 255), -1); // 빨간 점 유지
    }
}

// 에러 계산 함수
int getError(const Mat& frame, const Point& po) {
    return (frame.cols / 2 - po.x);
}

// Dynamixel 제어 함수
void controlDynamixel(Dxl &dxl, bool motor_active, double error, int &vel1, int &vel2) {
    const int MAX_ERROR = 300; // 최대 에러 값 제한
    error = (error > MAX_ERROR) ? MAX_ERROR : ((error < -MAX_ERROR) ? -MAX_ERROR : error);
    //double gain = (abs(error) > 200) ? 0.2 : 0.13; // 100기준 에러 값에 따라 게인 설정
    double gain = (abs(error) > 200) ? 0.19 : 0.11;
    if (motor_active) {
        //vel1 = 100 - gain * error;  // 왼쪽 바퀴 속도
        //vel2 = -(100 + gain * error); // 오른쪽 바퀴 속도
        vel1 = 200 - gain * error;  // 왼쪽 바퀴 속도
        vel2 = -(200 + gain * error); // 오른쪽 바퀴 속도
        limitVelocity(vel1, vel2); // 역방향 회전 방지
        dxl.setVelocity(vel1, vel2); // 속도 적용
    } else {
        vel1 = vel2 = 0; // 모터 정지
        dxl.setVelocity(vel1, vel2);
    }
}
// 속도 제한 함수
void limitVelocity(int &vel1, int &vel2) {
    if (vel1 < 0) vel1 = 130;
    if (vel2 > 0) vel2 = -130;
}
// 속도 제한 함수
void limitVelocity(int &vel1, int &vel2) {
    if (vel1 < 0) vel1 = 30;
    if (vel2 > 0) vel2 = -30;
}
