#ifndef VISION_HPP
#define VISION_HPP

#include <opencv2/opencv.hpp>
#include <vector>
using namespace std;
using namespace cv;

void preprocess(VideoCapture& source, Mat& frame, Mat& gray, Mat& thresh);// 전처리 함수: 입력 영상을 그레이스케일로 변환하고 이진화 및 자르기 처리
void findObjects(const Mat& thresh, Point& tmp_pt, Mat& result, Mat& stats, Mat& centroids);// 라인 후보 객체를 찾는 함수
void drawObjects(const Mat& stats, const Mat& centroids, const Point& tmp_pt, Mat& result);// 객체를 표시하는 함수
int getError(const Mat& frame, const Point& po);//에러값 계산 함수
void controlDynamixel(Dxl &dxl, bool motor_active, double error, int &vel1, int &vel2)//다이나믹셀 제어 함수
void limitVelocity(int &vel1, int &vel2);//다이나믹셀 역방향 제어 함수

#endif
