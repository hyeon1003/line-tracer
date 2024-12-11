#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "vision.hpp"  // 영상 처리 및 Dynamixel 제어 함수 포함
#include "dxl.hpp"     // Dynamixel 제어 라이브러리

#define K 0.15 // 게인 값 (기준 속도 100/-100에 맞춘 초기 값)
using namespace std;
using namespace cv;

// Ctrl+C 신호를 처리하기 위한 변수와 핸들러 함수
bool ctrl_c_pressed = false;
void ctrlc_handler(int) { ctrl_c_pressed = true; }

int main() {
    // 동영상 입력 파일 경로
    string input = "7_lt_ccw_100rpm_in.mp4";
    VideoCapture source(input);
    if (!source.isOpened()) {
        cerr << "Video open failed!" << endl;
        return -1;
    }

    // Dynamixel 초기화
    Dxl dxl;
    if (!dxl.open()) {
        cerr << "Dynamixel open error" << endl;
        return -1;
    }

    signal(SIGINT, ctrlc_handler); // Ctrl+C 신호 핸들러 설정

    // 주요 변수 초기화
    TickMeter tm;
    bool first_run = true;
    Point tmp_pt;
    Mat frame, gray, thresh, result, stats, centroids;
    double error;
    int vel1 = 0, vel2 = 0;
    bool motor_active = false;

    while (true) {
        tm.start(); // 처리 시간 측정 시작

        // 입력 영상 처리
        preprocess(source, frame, gray, thresh);
        if (thresh.empty()) break; // 프레임 없을 경우 종료
        if (first_run) {
            tmp_pt = Point(thresh.cols / 2, thresh.rows - 1);
            first_run = false;
        }
        findObjects(thresh, tmp_pt, result, stats, centroids); // 라인 검출
        drawObjects(stats, centroids, tmp_pt, result);         // 라인 시각화
        error = getError(result, tmp_pt);                     // 에러 계산

        if (dxl.kbhit()) { // 키 입력 처리
            char c = dxl.getch();
            if (c == 's') { // 's' 키 입력 시 모터 활성화/비활성화 토글
                motor_active = !motor_active;
                cout << (motor_active ? "Motor activated!" : "Motor deactivated!") << endl;
            }
        }

        // Dynamixel 제어 함수 호출
        controlDynamixel(dxl, motor_active, error, K, vel1, vel2);

        // 디버깅 출력
        tm.stop();
        cout << "Error: " << (int)error << "\tLeft: " << vel1 << "\tRight: " << vel2
             << "\tTime: " << tm.getTimeMilli() << " ms" << endl;
        tm.reset();

        // Ctrl+C 입력 처리
        if (ctrl_c_pressed) break;
    }

    dxl.close(); // Dynamixel 종료
    return 0;
}


