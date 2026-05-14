#pragma once
#include <iostream>
#include <numeric>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <ranges>
#include <filesystem>

inline auto
Test_virtual_painter() -> void
{
  // 虚拟画家
  // 通过定义的颜色过滤器在摄像机中实时检测对应颜色物体, 并框住,
  // 同时映射myColors物体到myColorValues得到画笔, 可以移动物体书写绘制实时的文字

  // hmin, smin, vmin hmax, smax, vmax
  static const std::array myColors {
    std::array { 124, 48, 117, 143, 170, 255 }, // Purple
    std::array { 68,  72, 156, 102, 126, 255 }, // Green
    std::array { 0,   62, 0,   35,  255, 255 }  // Orange
  };
  static const std::array myColorValues {
    cv::Scalar { 255, 0,   255 }, // Purple
    cv::Scalar { 0,   255, 0   }, // Green
    cv::Scalar { 51,  153, 255 }  // Orange
  };

  struct ColorPointMap
  {
    int x, y, idx;
  };

  std::vector<ColorPointMap> newPointMaps;

  cv::VideoCapture cap(0);
  if (!cap.isOpened())
    return;

  auto detectFunc = [&](cv::Mat img)
  {
    cv::Mat imgHSV;
    cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);
    // findColor
    for (auto const &[i, colors] : myColors | std::views::enumerate)
    {
      cv::Scalar lower(colors[0], colors[1], colors[2]);
      cv::Scalar upper(colors[3], colors[4], colors[5]);
      cv::Mat    mask;
      cv::inRange(imgHSV, lower, upper, mask);
      /*cv::imshow(std::to_string((std::inclusive_scan(colors.begin(), colors.end(), std::multiplies<> {}), colors.back())),
               imgHSV);*/
      cv::Point myPoint;

      // set point maps
      {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i>              hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point>> conPolyes(contours.size());
        std::vector<cv::Rect>               boundingBox(contours.size());

        for (auto const &[j, contour] : contours | std::views::enumerate)
        {
          if (cv::contourArea(contour) < 1000)
            continue;

          float peri = cv::arcLength(contour, true);
          cv::approxPolyDP(contour, conPolyes[j], 0.02 * peri, true);
          boundingBox[j] = cv::boundingRect(conPolyes[j]);

          myPoint.x = boundingBox[j].x + boundingBox[j].width / 2;
          myPoint.y = boundingBox[j].y;

          cv::drawContours(img, conPolyes, -1, cv::Scalar(255, 0, 255), 2);
          cv::rectangle(img, boundingBox[j].tl(), boundingBox[j].br(), cv::Scalar(0, 255, 0), 5);
        }
      }
      if (myPoint.x != 0 && myPoint.y != 0)
      {
        newPointMaps.emplace_back(myPoint.x, myPoint.y, i);
      }
    }

    // drawOnCanvas
    for (auto const &[x, y, idx] : newPointMaps)
    {
      cv::circle(img, { x, y }, 10, myColorValues[idx], cv::FILLED);
    }
  };

  cv::Mat img;
  while (true)
  {
    cap.read(img);

    detectFunc(img);

    cv::imshow("Image", img);

    if (cv::getWindowProperty("Image", cv::WND_PROP_VISIBLE) == 0)
      break;
    if (cv::waitKey(1) == 27)
      break;
  }
}

/*
// 预处理
  // 获取文件矩形位置的四个点
  // 使用red圆形标记四个点并绘制，观察顺序
  // 给四个点进行排序（如何实现算法?坐标求和微分?），得到Z顺序，左上，右上，左下，右下
  // warp投影倾斜矫正变换
  // crop裁剪一定的边距观感更紧凑贴合
*/
inline auto
Test_scan_document() -> void
{
  // 1. 读取图像
  cv::Mat img = cv::imread(PROJECT_DIRECTORY "/Resources/paper.jpg");
  if (img.empty())
  {
    std::cerr << "Failed to load paper.jpg" << std::endl;
    return;
  }

  // 2. 预处理：转为灰度、高斯模糊、边缘检测、膨胀闭合
  cv::Mat gray, blurred, edges, dilated;
  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
  cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
  cv::Canny(blurred, edges, 50, 150);
  cv::dilate(edges, dilated, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

  // 3. 寻找轮廓，获取最大面积矩形（假设文档是最大轮廓）
  std::vector<std::vector<cv::Point>> contours;
  std::vector<cv::Vec4i>              hierarchy;
  cv::findContours(dilated, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

  // 按面积降序排序
  std::sort(contours.begin(), contours.end(), [](auto &a, auto &b)
  {
    return cv::contourArea(a) > cv::contourArea(b);
  });

  if (contours.empty())
  {
    std::cerr << "No contour found!" << std::endl;
    return;
  }

  // 近似多边形，找到四边形
  std::vector<cv::Point2f> docCorners; // 存储文档的四个角点
  for (auto &contour : contours)
  {
    double                 peri = cv::arcLength(contour, true);
    std::vector<cv::Point> approx;
    cv::approxPolyDP(contour, approx, 0.02 * peri, true);
    if (approx.size() == 4)
    {
      docCorners.assign(approx.begin(), approx.end());
      break;
    }
  }

  if (docCorners.size() != 4)
  {
    std::cerr << "Failed to detect four corners of the document" << std::endl;
    return;
  }

  // 4. 将四个点按 Z 顺序排序（左上、右上、左下、右下）
  // 算法：根据 x+y 和 x-y 排序
  // 左上：x+y 最小
  // 右下：x+y 最大
  // 右上：x-y 最大（因为 x 大 y 小）
  // 左下：x-y 最小（因为 x 小 y 大）
  std::sort(docCorners.begin(), docCorners.end(), [](cv::Point2f a, cv::Point2f b)
  {
    return (a.x + a.y) < (b.x + b.y);
  });
  cv::Point2f topLeft     = docCorners[0];
  cv::Point2f bottomRight = docCorners[3];

  // 剩下两个点，根据 x-y 区分右上和左下
  cv::Point2f topRight, bottomLeft;
  cv::Point2f remaining1 = docCorners[1];
  cv::Point2f remaining2 = docCorners[2];
  if ((remaining1.x - remaining1.y) > (remaining2.x - remaining2.y))
  {
    topRight   = remaining1;
    bottomLeft = remaining2;
  }
  else
  {
    topRight   = remaining2;
    bottomLeft = remaining1;
  }

  std::vector<cv::Point2f> srcSorted = { topLeft, topRight, bottomLeft, bottomRight };

  // 5. 定义目标矩形的大小（基于原始图像宽高比例，取最大宽度和高度）
  float widthTop    = cv::norm(topRight - topLeft);
  float widthBottom = cv::norm(bottomRight - bottomLeft);
  float maxWidth    = std::max(widthTop, widthBottom);

  float heightLeft  = cv::norm(bottomLeft - topLeft);
  float heightRight = cv::norm(bottomRight - topRight);
  float maxHeight   = std::max(heightLeft, heightRight);

  std::vector<cv::Point2f> dstSorted = { cv::Point2f(0, 0), cv::Point2f(maxWidth, 0), cv::Point2f(0, maxHeight),
                                         cv::Point2f(maxWidth, maxHeight) };

  // 6. 透视变换
  cv::Mat transformMatrix = cv::getPerspectiveTransform(srcSorted, dstSorted);
  cv::Mat warped;
  cv::warpPerspective(img, warped, transformMatrix, cv::Size(maxWidth, maxHeight));

  // 7. 可选：裁剪边缘（去掉5像素黑边）
  int crop = 5;
  if (warped.rows > 2 * crop && warped.cols > 2 * crop)
  {
    warped = warped(cv::Rect(crop, crop, warped.cols - 2 * crop, warped.rows - 2 * crop));
  }

  // 8. 在原图上绘制四个角点（红色圆点）并显示
  cv::Mat imgWithCorners = img.clone();
  for (auto &p : srcSorted)
  {
    cv::circle(imgWithCorners, p, 8, cv::Scalar(0, 0, 255), cv::FILLED);
    // 可选：标注序号（调试用）
    // cv::putText(imgWithCorners, std::to_string(i), p, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0), 2);
  }

  cv::imshow("Original with corners", imgWithCorners);
  cv::imshow("Scanned Document", warped);
  cv::waitKey(0);
}

/*
函数说明
    级联分类器：使用 haarcascade_russian_plate_number.xml 检测车牌位置。
    车牌矫正：对每个检测到的矩形区域扩大边界，通过边缘检测和四边形近似，使用透视变换将车牌拉正，最后紧凑裁剪（去除5像素黑边）。
    保存策略：如果 ocrLicense 返回非空字符串，则文件名格式为 识别文字_时间戳.jpg，否则仅 时间戳.jpg。
    （OCR 函数已预留，需要时可接入 Tesseract 等库实现）
    实时显示：在摄像图像上绘制绿色矩形框，并单独显示矫正后的小图窗口。
    退出方式：按 ESC 键或关闭显示窗口。
*/
inline auto
Test_license_plate_detection() -> void
{
  // ---------- 1. 加载级联分类器 ----------
  cv::CascadeClassifier plateCascade;
  std::string           cascadePath = PROJECT_DIRECTORY "/Resources/haarcascade_russian_plate_number.xml";
  if (!plateCascade.load(cascadePath))
  {
    std::cerr << "Failed to load cascade: " << cascadePath << std::endl;
    return;
  }

  // ---------- 2. 打开摄像头 ----------
  cv::VideoCapture cap(0);
  if (!cap.isOpened())
  {
    std::cerr << "Cannot open camera" << std::endl;
    return;
  }

  // 保存目录
  std::string saveDir = PROJECT_DIRECTORY "/Resources/Plates/";
  // 确保目录存在（按题目描述已存在，但仍做安全检查）
  if (!std::filesystem::exists(saveDir))
  {
    std::filesystem::create_directories(saveDir);
  }

  // ---------- 辅助函数：矫正车牌（透视变换） ----------
  auto correctPlate = [](const cv::Mat &src, const cv::Rect &rect) -> cv::Mat
  {
    // 扩大检测框范围，避免边缘缺失
    cv::Rect expandedRect = rect;
    expandedRect.x        = std::max(0, rect.x - rect.width * 0.1);
    expandedRect.y        = std::max(0, rect.y - rect.height * 0.1);
    expandedRect.width    = std::min(src.cols - expandedRect.x, (int) (rect.width * 1.2));
    expandedRect.height   = std::min(src.rows - expandedRect.y, (int) (rect.height * 1.2));

    cv::Mat roi = src(expandedRect).clone();
    if (roi.empty())
      return cv::Mat();

    // 灰度化 + 边缘检测
    cv::Mat gray, edges;
    cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 0);
    cv::Canny(gray, edges, 50, 150);

    // 膨胀闭合细小间隙
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(edges, edges, kernel);
    cv::erode(edges, edges, kernel);

    // 寻找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty())
      return cv::Mat();

    // 按面积排序，取最大轮廓
    std::sort(contours.begin(), contours.end(), [](auto &a, auto &b)
    {
      return cv::contourArea(a) > cv::contourArea(b);
    });
    std::vector<cv::Point> approx;
    double                 peri = cv::arcLength(contours[0], true);
    cv::approxPolyDP(contours[0], approx, 0.02 * peri, true);

    // 需要四边形
    if (approx.size() != 4)
      return cv::Mat();

    // 将四个点按 Z 顺序排序（左上、右上、左下、右下）
    std::vector<cv::Point2f> srcPoints(4);
    for (int i = 0; i < 4; ++i)
      srcPoints[i] = approx[i];
    std::sort(srcPoints.begin(), srcPoints.end(), [](const cv::Point2f &a, const cv::Point2f &b)
    {
      return (a.x + a.y) < (b.x + b.y);
    });
    cv::Point2f tl = srcPoints[0];
    cv::Point2f br = srcPoints[3];
    cv::Point2f tr, bl;
    if ((srcPoints[1].x - srcPoints[1].y) > (srcPoints[2].x - srcPoints[2].y))
    {
      tr = srcPoints[1];
      bl = srcPoints[2];
    }
    else
    {
      tr = srcPoints[2];
      bl = srcPoints[1];
    }
    srcPoints = { tl, tr, bl, br };

    // 目标矩形大小（根据四边形的边长计算宽高）
    float widthTop    = cv::norm(tr - tl);
    float widthBottom = cv::norm(br - bl);
    float maxWidth    = std::max(widthTop, widthBottom);
    float heightLeft  = cv::norm(bl - tl);
    float heightRight = cv::norm(br - tr);
    float maxHeight   = std::max(heightLeft, heightRight);

    std::vector<cv::Point2f> dstPoints = { cv::Point2f(0, 0), cv::Point2f(maxWidth, 0), cv::Point2f(0, maxHeight),
                                           cv::Point2f(maxWidth, maxHeight) };

    // 透视变换
    cv::Mat transform = cv::getPerspectiveTransform(srcPoints, dstPoints);
    cv::Mat corrected;
    cv::warpPerspective(roi, corrected, transform, cv::Size(maxWidth, maxHeight));

    // 裁剪边缘（去掉5像素黑边）
    if (corrected.rows > 10 && corrected.cols > 10)
    {
      corrected = corrected(cv::Rect(5, 5, corrected.cols - 10, corrected.rows - 10));
    }
    return corrected;
  };

  // ---------- 预留 OCR 函数（返回识别的车牌字符串） ----------
  auto ocrLicense = [](const cv::Mat &plateImg) -> std::string
  {
    // TODO: 接入 Tesseract 或其他 OCR 引擎
    // 这里仅占位，返回空字符串表示未识别
    (void) plateImg;
    return "";
  };

  // ---------- 生成时间戳字符串 ----------
  auto getTimestamp = []() -> std::string
  {
    auto        now = std::chrono::system_clock::now();
    auto        ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t   = std::chrono::system_clock::to_time_t(now);
    std::tm     tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
  };

  // ---------- 主循环 ----------
  cv::Mat     frame;
  std::string winname = "License Plate Detection";
  cv::namedWindow(winname, cv::WINDOW_NORMAL);

  while (true)
  {
    cap.read(frame);
    if (frame.empty())
      break;

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // 检测车牌
    std::vector<cv::Rect> plates;
    plateCascade.detectMultiScale(gray, plates, 1.1, 5, 0, cv::Size(60, 20));

    for (size_t i = 0; i < plates.size(); ++i)
    {
      cv::Rect r = plates[i];
      // 画原始检测框
      cv::rectangle(frame, r, cv::Scalar(0, 255, 0), 2);

      // 矫正车牌
      cv::Mat correctedPlate = correctPlate(frame, r);
      if (!correctedPlate.empty())
      {
        // 可选：OCR 识别
        std::string plateText = ocrLicense(correctedPlate);
        std::string filename  = saveDir;
        if (!plateText.empty())
        {
          filename += plateText + "_" + getTimestamp() + ".jpg";
        }
        else
        {
          filename += getTimestamp() + ".jpg";
        }
        cv::imwrite(filename, correctedPlate);
        std::cout << "Saved: " << filename << std::endl;

        // 在原始图像上显示矫正后的小图（可选）
        cv::Mat small;
        cv::resize(correctedPlate, small, cv::Size(100, 30));
        cv::imshow("Corrected Plate", small);
      }
    }

    cv::imshow(winname, frame);

    if (cv::getWindowProperty(winname, cv::WND_PROP_VISIBLE) == 0)
      break;
    if (cv::waitKey(1) == 27)
      break;
  }

  cv::destroyWindow(winname);
}
