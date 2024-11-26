#include "vision.hpp"

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
    threshold(gray, thresh, 130, 255, THRESH_BINARY);

    // 관심 영역(ROI) 자르기: 영상의 하단 1/4만 남김
    int r_pts = thresh.rows / 4 * 3;
    Rect r(0, r_pts, thresh.cols, thresh.rows - r_pts);
    thresh = thresh(r);
}

void findObjects(const Mat& thresh, Point& tmp_pt, Mat& result, Mat& stats, Mat& centroids) {
    Mat labels;
    int cnt = connectedComponentsWithStats(thresh, labels, stats, centroids);

    // 1채널 -> 3채널 변환 (디버깅용)
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

            if (dist < min_dist && dist <= 150) {
                min_dist = dist;
                min_index = i;
            }
        }
    }

    if (min_index != -1 && min_dist <= 150) {
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

int getError(const Mat& result, const Point& tmp_pt) {
    return (result.cols / 2) - tmp_pt.x;
}
