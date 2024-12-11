#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "vision.hpp"  // 영상 처리 및 Dynamixel 제어 함수 포함
#include "dxl.hpp"     // Dynamixel 제어 라이브러리

//#define K 0.15 // 게인 값 (기준 속도 100/-100에 맞춘 초기 값)
using namespace std;
using namespace cv;

// Ctrl+C 신호를 처리하기 위한 변수와 핸들러 함수
bool ctrl_c_pressed = false;
void ctrlc_handler(int) { ctrl_c_pressed = true; }

int main() {
    // 동영상 입력 파일 경로
    // ccw in 7_lt_ccw_100rpm_in.mp4
    // out 5_lt_cw_100rpm_out.mp4
    string input = "7_lt_ccw_100rpm_in.mp4";
    VideoCapture source(input);
    //jetson 카메라
    /*string src = "nvarguscamerasrc sensor-id=0 ! \
	video/x-raw(memory:NVMM), width=(int)640, height=(int)360, \
    format=(string)NV12, framerate=(fraction)30/1 ! \
	nvvidconv flip-method=0 ! video/x-raw, \
	width=(int)640, height=(int)360, format=(string)BGRx ! \
	videoconvert ! video/x-raw, format=(string)BGR ! appsink";
    */

    //VideoCapture source(src);  // 동영상 파일 읽기
    if (!source.isOpened()) {  // 파일 열기 실패 시 종료
        cerr << "Video open failed!" << endl;
        return -1;
    }
    // GStreamer를 사용한 영상 스트리밍 설정 (결과 전송)
    string dst1 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8001 sync=false";
    VideoWriter writer1(dst1, 0, (double)30, Size(640, 360), true);  // 전송 스트림 1
    string dst2 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8002 sync=false";
    VideoWriter writer2(dst2, 0, (double)30, Size(640, 360), false);  // 전송 스트림 2
    string dst3 = "appsrc ! videoconvert ! video/x-raw, format=BGRx ! \
    nvvidconv ! nvv4l2h264enc insert-sps-pps=true ! \
    h264parse ! rtph264pay pt=96 ! \
    udpsink host=203.234.58.166 port=8003 sync=false";
    VideoWriter writer3(dst3, 0, (double)30, Size(640, 90), true);  // 전송 스트림 3
    if (!writer1.isOpened() || !writer2.isOpened()) {  // 스트리밍 실패 시 종료
        cerr << "Failed to open GStreamer video writers!" << endl;
        return -1;
    }
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
        controlDynamixel(dxl, motor_active, error,vel1, vel2);
        writer1<<frame;  
        writer2<<gray;
        writer3<<result;
        usleep(20 * 1000);
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
