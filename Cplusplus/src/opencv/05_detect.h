#include <opencv2/freetype.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <print>
#include <ranges>

inline auto
Test_detect_color() -> void
{
  // HSV颜色侦测

  cv::Mat imgHSV, mask;
  //shapes.png可选查看每个颜色在哪一个HSV范围被过滤
  cv::Mat img { cv::imread(PROJECT_DIRECTORY "/Resources/lambo.png") };
  cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);

  int hmin = 0, smin = 0, vmin = 0;
  int hmax = 179, smax = 255, vmax = 255;

  // 创建滚动条调整HSV标量
  cv::namedWindow("Trackbars", (640, 200));
  cv::createTrackbar("Hue Min", "Trackbars", &hmin, 179);
  cv::createTrackbar("Hue Max", "Trackbars", &hmax, 179);
  cv::createTrackbar("Sat Min", "Trackbars", &smin, 255);
  cv::createTrackbar("Sat Max", "Trackbars", &smax, 255);
  cv::createTrackbar("Val Min", "Trackbars", &vmin, 255);
  cv::createTrackbar("Val Max", "Trackbars", &vmax, 255);


  while (true)
  {
    cv::Scalar_<int> lower(hmin, smin, vmin);
    cv::Scalar_<int> upper(hmax, smax, vmax);

    // 频率范围限制
    cv::inRange(imgHSV, lower, upper, mask);

    cv::imshow("Image", img);
    cv::imshow("Image HSV", imgHSV);
    cv::imshow("Image Mask", mask);
    cv::waitKey(1);
  }
}

inline auto
Test_detect_camera_color_and_output() -> void
{
  int hmin = 0, smin = 0, vmin = 0;
  int hmax = 179, smax = 255, vmax = 255;

  cv::namedWindow("Trackbars", cv::WINDOW_NORMAL);
  cv::createTrackbar("Hue Min", "Trackbars", &hmin, 179);
  cv::createTrackbar("Hue Max", "Trackbars", &hmax, 179);
  cv::createTrackbar("Sat Min", "Trackbars", &smin, 255);
  cv::createTrackbar("Sat Max", "Trackbars", &smax, 255);
  cv::createTrackbar("Val Min", "Trackbars", &vmin, 255);
  cv::createTrackbar("Val Max", "Trackbars", &vmax, 255);

  cv::VideoCapture cap(0);
  if (!cap.isOpened())
    return;

  cv::Mat           img, imgHSV, mask;
  const std::string winname = "Image";
  cv::namedWindow(winname);

  while (true)
  {
    cap.read(img);
    if (img.empty())
    {
      std::cerr << "Empty frame, exiting..." << std::endl;
      break;
    }

    cv::cvtColor(img, imgHSV, cv::COLOR_BGR2HSV);

    cv::Scalar lower(hmin, smin, vmin);
    cv::Scalar upper(hmax, smax, vmax);
    std::println("{},{},{},{},{},{}", hmin, smin, vmin, hmax, smax, vmax);
    cv::inRange(imgHSV, lower, upper, mask);

    cv::imshow(winname, img);
    cv::imshow("Image Mask", mask);

    if (cv::getWindowProperty(winname, cv::WND_PROP_VISIBLE) == 0)
      break;
    if (cv::waitKey(1) == 27)
      break;
  }

  cv::destroyWindow(winname);
}

inline auto
Test_detect_shape() -> void
{
#if _WIN32
  setlocale(LC_ALL, ".utf-8");
#endif
  cv::Ptr<cv::freetype::FreeType2> ft2 = cv::freetype::createFreeType2();
  // 请确保字体文件路径正确
  ft2->loadFontData("C:/Windows/Fonts/simhei.ttf", 0);

  cv::Mat img { cv::imread(PROJECT_DIRECTORY "/Resources/shapes.png") };
  cv::Mat imgGray, imgBlur, imgCanny, imgDilate, imgErode;
  // 灰度转换
  cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);
  // 高斯模糊
  cv::GaussianBlur(img, imgBlur, { 3, 3 }, 3, 0);
  // 边缘检测
  cv::Canny(imgBlur, imgCanny, 25, 75);
  // 膨胀, 线厚度增加
  cv::dilate(imgCanny, imgDilate, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));
  // 腐蚀, 线厚度减小
  cv::erode(imgCanny, imgErode, cv::getStructuringElement(cv::MORPH_RECT, { 3, 3 }));

  // 膨胀效果更好, 我们传入 imgDilate 获取形状轮廓

  //cv::imshow("Image Gray", imgGray);
  //cv::imshow("Image Blur", imgBlur);
  //cv::imshow("Image Canny", imgCanny);
  //cv::imshow("Image Dilate", imgDilate);
  //cv::imshow("Image Erode", imgErode);

  [](cv::Mat dil, cv::Mat img) noexcept
  {
    /*
    输出：检测到的轮廓列表。类型为 std::vector<std::vector<cv::Point>>。
         每个轮廓是一个 vector<cv::Point>, 存储了该轮廓上点的序列。
    */
    std::vector<std::vector<cv::Point>> contours;
    /*
    输出：轮廓的层次结构信息。类型为 std::vector<cv::Vec4i>, 大小与 contours 相同
         每个元素 hierarchy[i] 包含4个 int 值：
         - hierarchy[i][0]：同层级下一条轮廓的索引（next）
         - hierarchy[i][1]：同层级上一条轮廓的索引（previous）
         - hierarchy[i][2]：第一个子轮廓的索引（first child）
         - hierarchy[i][3]：父轮廓的索引（parent）
         若某项不存在, 值为 -1。
    */
    std::vector<cv::Vec4i> hierarchy;

    /*
    轮廓检索模式：
    - cv::RETR_EXTERNAL：只检索最外层轮廓, 忽略内部孔洞或嵌套轮廓。适合不需要内部结构的场景。
    - 其他常用模式：RETR_TREE（完整层次结构）、RETR_LIST（无层次）、RETR_CCOMP（两级层次）等。
    轮廓近似方法：
    - cv::CHAIN_APPROX_SIMPLE：压缩水平、垂直、对角线方向的连续点, 只保留端点（例如矩形仅存4个点）。
    - cv::CHAIN_APPROX_NONE：保存轮廓上所有连续点。
    */
    cv::findContours(dil, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    std::vector<std::vector<cv::Point>> conPolyes(contours.size());
    std::vector<cv::Rect>               boundingBox(contours.size());
    for (auto const &[index, contour] : contours | std::views::enumerate)
    {
      // 过滤面积小于1000的形状, 我们不绘制轮廓
      if (cv::contourArea(contour) < 1000)
        continue;

      /*
      * 计算轮廓弧长
      第二个参数（bool closed）：指定轮廓是否闭合。
      - true：将首尾点相连，计算封闭曲线的总长度。
      - false：按点序计算折线长度，首尾不相连。
      */
      float peri = cv::arcLength(contour, true);

      // 第二个参数：输出近似后的多边形点集
      /*
      epsilon – 近似的精度参数。
      - 表示原始轮廓到近似多边形的最大允许距离（像素）。
      - 值越小，近似多边形越精细（顶点越多）；值越大，简化越厉害（顶点越少）。
      - 这里取周长的 0.02（2%），是个常用比例，能较好保留主要形状的同时减少噪声影响。
      */
      cv::approxPolyDP(contour, conPolyes[index], 0.02 * peri, true);

      /*
      绘制boundingBox
      */
      boundingBox[index] = cv::boundingRect(conPolyes[index]);
      cv::rectangle(img, boundingBox[index].tl(), boundingBox[index].br(), cv::Scalar(0, 255, 0), 5);

      /*
      要绘制的轮廓索引：
      - -1：绘制所有轮廓。
      - 其他非负整数：只绘制指定索引的那一条轮廓。
      */
      cv::drawContours(img, conPolyes, -1, cv::Scalar(255, 0, 255), 2);

      std::string detectShape;
      switch (conPolyes[index].size())
      {
      case 3 : detectShape = "三角形"; break;
      case 4 :
      {
        if (float aspRatio = (float) boundingBox[index].width / boundingBox[index].height;
            aspRatio >= 0.95f && aspRatio <= 1.05f)
        {
          detectShape = "正方形";
        }
        else
        {
          detectShape = "矩形";
        }
        break;
      }
      default : detectShape = "多边形"; break;
      }
      // 绘制文字必须在绘制轮廓之前，否则将被覆盖不可见
      cv::putText(img, detectShape, boundingBox[index].tl() - cv::Point { 0, 5 }, cv::FONT_HERSHEY_DUPLEX, 0.75,
                  { 0, 69, 255 }, 2);
    }
  }(imgDilate, img);

  cv::imshow("Image img", img);
  cv::waitKey();
}


inline auto 
Test_detect_human_face()
{
  // 使用训练好的XML进行人脸矩形框定位
  cv::Mat               img { cv::imread(PROJECT_DIRECTORY "/Resources/test.png") };
  cv::CascadeClassifier faceCascade;
  faceCascade.load("Resources/haarcascade_frontalface_default.xml");
  if (faceCascade.empty())
  {
    return -1;
  }

  std::vector<cv::Rect> faces;
  faceCascade.detectMultiScale(img, faces, 1.1, 10);
  for (auto const &face : faces)
  {
    cv::rectangle(img, face.tl(), face.br(), cv::Scalar(255, 0, 255), 3);
  }

  cv::imshow("Image", img);
  cv::waitKey();
}
