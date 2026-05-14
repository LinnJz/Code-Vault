#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

auto
Test_op1() ->void
{
  // 预处理

  cv::Mat img { cv::imread(PROJECT_DIRECTORY "/Resources/test.png") };
  cv::Mat imgGray, imgBlur, imgCanny, imgDilate, imgErode;
  // 灰度转换
  cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);
  // 高斯模糊
  cv::GaussianBlur(img, imgBlur, { 7, 0 }, 5, 0);
  // 边缘检测
  cv::Canny(imgBlur, imgCanny, 25, 75);
  // 膨胀, 线厚度增加
  cv::dilate(imgCanny, imgDilate, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));
  // 腐蚀, 线厚度减小
  cv::erode(imgCanny, imgErode, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));

  cv::imshow("Image Gray", imgGray);
  cv::imshow("Image Blur", imgBlur);
  cv::imshow("Image Canny", imgCanny);
  cv::imshow("Image Dilate", imgDilate);
  cv::imshow("Image Erode", imgErode);

  cv::waitKey();
}

auto
Test_op2() -> void
{
  cv::Mat img { cv::imread(PROJECT_DIRECTORY "/Resources/test.png") };
  cv::Mat imgResize1, imgResize2, imgCrop;
  // 拉伸/挤压宽高
  cv::resize(img, imgResize1, { 1080, 720 });
  // 等比缩放宽高
  cv::resize(img, imgResize2, {}, 0.5, 0.5);
  // 裁剪, 根据矩形roi区域
  imgCrop = img(cv::Rect { 100, 100, 300, 250 });

  cv::imshow("Image Resize1", imgResize1);
  cv::imshow("Image Resize2", imgResize2);
  cv::imshow("Image Crop", imgCrop);

  cv::waitKey();
}
