#pragma once

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

inline auto
Test_draw() -> void
{
  // 每像素0~255(8bit), 通道3, Scalar是标量代表颜色
  cv::Mat img {
    512, 512, CV_8UC3, cv::Scalar { 127, 127, 127 }
  };
  // 中心位置画圆, 并填充圆
  cv::circle(img, cv::Point { 256, 256 }, 155, cv::Scalar { 0, 69, 255 }, cv::FILLED);
  // 中心位置画矩形, 并填充
  cv::rectangle(img, cv::Point { 130, 226 }, cv::Point { 382, 286 }, cv::Scalar { 255, 255, 255 }, cv::FILLED);
  // 画线
  cv::line(img, cv::Point(130, 296), cv::Point(382, 296), cv::Scalar(255, 255, 255), 2);

  // 添加文本
  cv::putText(img, "Hello World!", cv::Point(137, 262), cv::FONT_HERSHEY_DUPLEX, 0.75, cv::Scalar(0, 69, 255), 2);

  cv::imshow("Image Draw", img);

  cv::waitKey();
}
