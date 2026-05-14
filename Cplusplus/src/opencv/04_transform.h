#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

inline auto
Test_warp() -> void
{
  // 将扑克牌矫正
  constexpr float w = 250.f, h = 350.f;

  cv::Mat     img { cv::imread(PROJECT_DIRECTORY "/Resources/cards.jpg") };
  cv::Point2f src[4] = {
    { 529, 142 },
    { 771, 190 },
    { 405, 395 },
    { 674, 457 }
  };
  cv::Point2f dst[4] = {
    { 0.0f, 0.0f },
    { w,    0.0f },
    { 0.0f, h    },
    { w,    h    }
  };
  // 变换得到透视矩阵
  auto matrix = cv::getPerspectiveTransform(src, dst);

  // 矫正
  cv::Mat imgWrap;
  cv::warpPerspective(img, imgWrap, matrix, cv::Point(w, h));
  constexpr auto s = std::size(src);
  // 标记定位点, 注意不要在矫正之前做, 因为这样矫正的点会携带标记
  for (auto i { 0uz }; i < std::extent_v<decltype(src)>; ++i)
  {
    cv::circle(img, src[i], 10, cv::Scalar(0, 0, 255), cv::FILLED);
  }

  cv::imshow("Image", img);
  cv::imshow("Image Wrap", imgWrap);
  cv::waitKey();
}
