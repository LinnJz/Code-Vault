#pragma once

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

inline auto
Test_img() -> void
{
  cv::Mat img { cv::imread(PROJECT_DIRECTORY "/Resources/test.png") };
  cv::imshow("Image", img);
  cv::waitKey();
}

inline auto
Test_video() -> void
{
  cv::VideoCapture cap { PROJECT_DIRECTORY "/Resources/test_video.mp4" };
  if (!cap.isOpened())
  {
    return;
  }
  cv::Mat img;
  while (cap.read(img))
  {
    cv::imshow("Image", img);
    if (cv::waitKey(20) == 27) // 27 是 ESC 的 ASCII 码
      break;
  }
}

inline auto
Test_camera() -> void
{
  cv::VideoCapture cap { 0 }; // number是摄像头个数编号, 使用哪个就填对应编号
  if (!cap.isOpened())
  {
    return;
  }
  cv::Mat            img;
  std::string const &winname = "Image";

  cv::namedWindow(winname);
  while (cap.read(img))
  {
    cv::imshow(winname, img);

    // 检查窗口是否被关闭（点击 X）
    if (cv::getWindowProperty(winname, cv::WND_PROP_VISIBLE) == 0)
      break;

    // 也可以支持按 ESC 键退出，更友好
    if (cv::waitKey(1) == 27) // ESC
      break;
  }

  cv::destroyWindow(winname);
}
