#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "vision.hpp"
#include "dxl.hpp"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// Ctrl+C 핸들러 설정
bool ctrl_c_pressed = false;
void ctrlc_handler(int) { ctrl_c_pressed = true; }

int main() {
    // 동영상 파일 경로 (나중에 GStreamer로 바꿔야 함)
    // in 8_lt_cw_100rpm_in.mp4
    // out 5_lt_cw_100rpm_out.mp4
    string input = "8_lt_cw_100rpm_in.mp4";
    VideoCapture source(input);
    // 동영상 파일 열기 확인
    if (!source.isOpened()) {
        cerr << "Video open failed!" << endl;
        return -1;
    }

    // GStreamer를 사용한 영상 전송 설정
    string dst1 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8001 sync=false";
    VideoWriter writer1(dst1, 0, (double)30, Size(640,360), true);
    string dst2 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8002 sync=false";
    VideoWriter writer2(dst2, 0, (double)30, Size(640,360), false);
    string dst3 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8003 sync=false";
    VideoWriter writer3(dst3, 0, (double)30, Size(640, 90), true);
    if (!writer1.isOpened() || !writer2.isOpened()) {
        cerr << "Failed to open GStreamer video writers!" << endl;
        return -1;
    }
    // Dynamixel 초기화
    Dxl dxl;
    if (!dxl.open()) {
        cerr << "Dynamixel open error" << endl;
        return -1;
    }

    signal(SIGINT, ctrlc_handler); // Ctrl+C 핸들러 설정

    TickMeter tm; // TickMeter로 실행 시간 측정
    bool first_run = true;
    Point tmp_pt;
    Mat frame, gray, thresh, result, stats, centroids;
    int error;      // 에러 값 저장
    int vel1 = 0;   // 왼쪽 바퀴 속도
    int vel2 = 0;   // 오른쪽 바퀴 속도
    double k = 0.5; // 게인 값
    bool motor_active = false; // Dynamixel 작동 여부 플래그

    while (true) {
        tm.start(); // 시간 측정 시작
        preprocess(source, frame, gray, thresh); // 프레임 전처리
        if (thresh.empty()) break; // 프레임이 없으면 종료
        if (first_run) {
            tmp_pt = Point(thresh.cols / 2, thresh.rows - 1); // 초기 위치 설정
            first_run = false;
        }
        findObjects(thresh, tmp_pt, result, stats, centroids); // 라인 탐지
        drawObjects(stats, centroids, tmp_pt, result);         // 라인 디버깅 표시
        // 에러 계산
        error = getError(result, tmp_pt);
        // 's' 키 입력 시 Dynamixel 활성화
        if (dxl.kbhit()) {
            char c = dxl.getch();
            //if( c=='q')break;
            if (c == 's') {
                motor_active = !motor_active; // 모터 활성화/비활성화 토글
                cout << (motor_active ? "Motor activated!" : "Motor deactivated!") << endl;
            }
        }

        if (motor_active) {
            // 에러 값을 기반으로 속도 설정
            vel1 = 100 -k*error;  // 기본 속도 100에서 에러를 반영
            vel2 = -(100 + k*error); 
            dxl.setVelocity(vel1, vel2);// Dynamixel 속도 명령 전송
        } 
        
        else {
            // 모터 비활성화 상태에서 속도 0 유지
            vel1 = 0;
            vel2 = 0;
            dxl.setVelocity(vel1, vel2);
        }
        // 영상 전송
        writer1<<frame;  
        writer2<<gray;
        writer3<<result;
        if (ctrl_c_pressed) break;// Ctrl+C 입력 시 종료
        tm.stop(); // 시간 측정 종료
        cout << "Error: " << error << "\tLeft: " << vel1 << "\tRight: " << vel2
             << "\tTime: " << tm.getTimeMilli() << " ms" << endl;

        tm.reset(); // 시간 측정 초기화
        usleep(20 * 1000); // 20ms 대기
    }
    dxl.close(); // Dynamixel 종료
    return 0;
}
