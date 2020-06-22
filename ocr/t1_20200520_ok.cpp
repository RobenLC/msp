/**********************************************************
Name :
Date : 2019/12/2
By   : liaojf
Final:
     : 2019/10/23: initial svm code
     : 2019/10/28: read 冠字號
	 : 2019/11/6 : show input image
	 : 2019/11/21 : try HOG feature
	 : 2019/12/9  : output decide character in order
	 : 2019/12/17 : add euler number, judge a list of images
	              : write result to one report file
     : 2019/12/24 : add center_weight
	 : 2019/12/30 : add dilation
	 : 2020/1/8   : deal img117-253 , img106-116
	 : 2020/1/21  : add report out
	 : 2020/1/22  : add y-pos constraint, add tw_base groups
   : 2020/2/6   : modify code for ubuntu  , path strings are different
   : 2020/3/9   : test speed up
   : 2020/3/9   : test org275_1 ~ org402_1
   : 2020/3/10  : test org403_1 ~ org428_1
   : 2020/3/11  : fontzie=[16,32], vector =108, kernel
   : 2020/3/12  : correct fignum value
   : 2020/3/12  : add left_cor to separate "B" + "8"
   : 2020/3/13  : improve accuracy
   : 2020/3/16  : for scan_flow test
   : 2020/3/16  : scan_flow test for DK2
   : 2020/3/17  : for LEO image (dark)
   : 2020/3/18  : remove large width font, height <20
   : 2020/3/19  : use NTU liblinear, 6 section codes
   : 2020/3/23  :  rm dummy image, dst, drawing
   : 2020/3/23  : remove dummy images ,dst, drawing
   : 2020/3/24  : remove bw
   : 2020/3/25  : use adaptive_thresh(51,20)
   : 2020/3/26  : font height >18
   : 2020/3/27  : add ming-image section7, handle shortage out_fonts
   : 2020/3/30  : equalizeHist,meanStdDev,calcHist: threshold value
   : 2020/3/31  : add section_9(img595~ img769)
   : 2020/4/1   : add section_10(img770~ img879)
   : 2020/4/8   : cropping 冠字號 block + section_11~12
   : 2020/4/9   : search top-left point from left-half-image area, bounding_rect.height >10
   : 2020/4/10  : defective  冠字號 problem
   : 2020/4/13  : left-top y-pos limit <55+ merged two images
   : 2020/4/14  : add F+I+O constraint; adaptive+fixed mixed mode
   : 2020/4/15  : AI training behavior :step1 : collect error fonts
                : add compare golden table
   : 2020/4/22  : Collect one 冠字號 set under no golden table for comparison
   : 2020/4/23  : denoise_img -> dilate -> invbw_img
   : 2020/4/24  : choose ocr error font into lib_model
   : 2020/4/27  : change center_pix conditon for "D"
   : 2020/4/28  : rm cvtColor
   : 2020/5/8   : 整合前級OCR + 後級收集error font, 重新產生model
   : 2020/5/19  : 先做OCR+ 從全錯字中湊出2組字庫
   : 2020/5/20  : for DK2, OCR => two font sets; disable ouptut msg
   : 2020/5/28  : close inout streams
**********************************************************/
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv/cv.h>
#include <iostream>
#include <fstream>
#include <vector>
#include "svm.h"
#include "linear.h"
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <math.h>
//#include <Windows.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//#pragma comment(lib,"libsvm.lib")


using namespace cv;
using namespace std;
using namespace cv::ml;

void CopyFile(char *source, char *dest)
{
    int childExitStatus;
    pid_t pid;
    int status;
    if (!source || !dest) {
        /* handle as you wish */
    }

    pid = fork();

    if (pid == 0) { /* child */
        execl("/bin/cp", "/bin/cp", source, dest, (char *)0);
    }
    else if (pid < 0) {
        /* error - couldn't start process - you decide how to handle */
    }
    else {
        /* parent - wait for child - this has all error handling, you
         * could just call wait() as long as you are only expecting to
         * have one child process at a time.
         */
        pid_t ws = waitpid( pid, &childExitStatus, WNOHANG);
        if (ws == -1)
        { /* error - handle as you wish */
        }

        if( WIFEXITED(childExitStatus)) /* exit code in childExitStatus */
        {
            status = WEXITSTATUS(childExitStatus); /* zero is normal exit */
            /* handle non-zero as you wish */
        }
        else if (WIFSIGNALED(childExitStatus)) /* killed */
        {
        }
        else if (WIFSTOPPED(childExitStatus)) /* stopped */
        {
        }
    }
}



vector<int> current_layer_holes(vector<Vec4i> layers, int index) {
	int next = layers[index][0];
	vector<int> indexes;
	indexes.push_back(index);
	while (next >= 0) {
		indexes.push_back(next);
		next = layers[next][0];
	}
	return indexes;
}


typedef struct  alpha_order{
	float x_pos;
	double decide_val;
	int eulernum;
	int center_wk;
	int left_cor;
	//--- 2020/4/15
	int imgnum;
} font_order;

bool struct_cmp_by_x_pos(font_order a, font_order b)
{
	return a.x_pos < b.x_pos;
}

typedef struct  body_order {
	float x_pos;
	float y_pos;
	float area;
} bd_order;

bool struct_cmp_blk_xy(bd_order a, bd_order b)
{
	if ((a.x_pos < b.x_pos))
		return true;
	else if ((a.x_pos == b.x_pos) && (a.y_pos < b.y_pos))
		return true;
	else
		return false;
}



/*----
 struct Color {
   int R;
   int G;
   int B;
 };
 --*/


#define debug_20191219
//#define single_test

#define tw_sec13   //---img1080-img-img1213


#define ubuntu
//#define win7
static struct model *model_ntu;
static int orghog_descr_size;
static int total_col_img;

static ofstream rpt_out;
static ofstream rpt_out2;
static ifstream imgnum_in;
static ifstream golden_in;
static ofstream rec_out;

#ifdef MAKELIB
int add_image_to_model( void * img_buf, int img_size, int state )
#else
int main(int argc, char **argv)
#endif
{
//char * main(){


    //-----切割 input 冠字號
    char *dest = "input%d_1.bmp";
    char basefilename[100] = " ";

    Mat dengray, thr_img;
    char fontname[100] = " ";
    static int k;
#ifdef MAKELIB
    cout << "makemodel --- " << state << "\n" << std::flush;
#else
    cout << "makemodel --- " << argc << "\n" << std::flush;
#endif

  //struct svm_model *model_ntu;
   //struct model *model_ntu;
#ifdef MAKELIB
    if( state==0 )
    {
#endif

  //model_ntu = svm_load_model("cv_tw_model.txt");
   model_ntu = load_model("cv_tw_model_linear.txt");
  //int orghog_descr_size = 4032;
   //  int orghog_descr_size = 576;
   orghog_descr_size = 108;
   total_col_img = 1;


#ifdef tw_sec13
   rpt_out.open("cv_det_tw_1080_1213.txt");
   rpt_out2.open("num_det_tw_1080_1213.txt");
   //--try read "num_det_tw_xxxx.txt"
   //---- 以知冠字號(號碼) , 對答案用
   imgnum_in.open("num_det_tw_1080_1213b.txt");
#endif


  if (!rpt_out) {
	  cout << "create cv_det_tw_1080_1213.txt fail !!!\n";
	 // return 1;
  }


 //---------2020/4/15  compare golden table
  //---- auto check

#ifdef tw_sec13

  //---- read golden 冠字號(字母)
  //ifstream golden_in("G:\\ljf_repos\\OpenCV_test\\test1_lin\\test1\\history_answer_database\\detect_tw_1080_1213_gray.txt");
  golden_in.open("detect_tw_1080_1213_gray_none.txt");
  //--- 記錄錯誤
  rec_out.open("tw_1080_1213_errorlist.txt");

#endif



  if (!golden_in) {
	  cout << "open detect_tw_1080_1213_gray_none.txt FAIL \n";
	  //return 1;
  }

#ifdef MAKELIB
    k=0;
}   // if( state==0 )
else if( state == 1)
{
    k++;

#else   // #ifdef MAKELIB

#ifdef tw_sec13
      for (int k = 1080; k < 1214; k++)
#endif


#ifdef single_test
 int k;
 while (cin >> k)
#endif

#endif  // #ifdef MAKELIB

  {


#ifdef MAKELIB
      {
          std::vector<unsigned char> data((unsigned char*)img_buf, (unsigned char*)img_buf + img_size);
          dengray = cv::imdecode(Mat(data), CV_LOAD_IMAGE_GRAYSCALE);
          data.clear();
      }
#else
    {
	  //------original golden font
    //--- for ubuntu
    #ifdef ubuntu

	 char *infont ;
    if( argc == 2 )
        infont = "/home/root/tw_base_hv/org%d_1.bmp";
    else
        infont = "/home/root/tw_base/org%d_1.bmp";
    #endif


	sprintf(fontname, infont, k);
      cout << fontname << "\n" <<  std::flush;

	  //----- 20200428 ok denimg = imread(fontname);
	  dengray = imread(fontname, CV_LOAD_IMAGE_GRAYSCALE);
	  //0206 imshow("original img", denimg);
    }
#endif  // #ifdef MAKELIB

	   /*
	   0: Binary
	   1: Binary Inverted
	   2: Threshold Truncated
	   3: Threshold to Zero
	   4: Threshold to Zero Inverted
	   */


	  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	  //------- 2020/4/7 find 冠字號 center
	  vector<vector<Point> > cen_contours;
	  vector<Vec4i> cen_hierarchy;
	  Rect  trace_rect;
	  Mat group_img;
	  vector<bd_order> char_area;
	  bd_order bd_pos;

	  //--- 2020/4/13 first, do whole image with fixed threshold
	  // Mat merge_img = dengray.clone();
	  Mat merge_img;
	  threshold(dengray, merge_img, 150, 255, THRESH_BINARY_INV);
	  //---- 20200409
	  //--- use left-half image
	  group_img = merge_img(Rect(0, 0, 65, merge_img.rows));


	  imwrite("group.bmp", group_img);

	  findContours(group_img, cen_contours, cen_hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	  //cout << "find font num = " << cen_contours.size() << endl;

	  //---- 2020/4/10 if char_area.size ==0, many points to be checked
	  if ( cen_contours.size() < 40) {

		  for (int k = 0; k < cen_contours.size(); k++) {
			  trace_rect = boundingRect(cen_contours[k]);
			  //cout << "2nd area calculation= " << contourArea(cen_contours[k], false) << endl;
			  //cout << "  x=" << trace_rect.x << " y=" << trace_rect.y << endl;

			  if (contourArea(cen_contours[k], false) > 6 &&
				  trace_rect.width < 27 && trace_rect.height >5
				  && trace_rect.y <55) {
				  bd_pos.x_pos = trace_rect.x;
				  bd_pos.y_pos = trace_rect.y;
				  bd_pos.area = contourArea(cen_contours[k], false);
				  char_area.push_back(bd_pos);
			  }

		  }
	  }

	  if (char_area.size() != 0) {
		  sort(char_area.begin(), char_area.end(), struct_cmp_blk_xy);
		  // cout << " 1st x=" << char_area[0].x_pos << "  1st y=" << char_area[0].y_pos << endl;

		  Mat focus_img;
		  //--- check out_of_range
		  int y_limit, x_limit;
		  int y_st_limit, x_st_limit;

		  if (char_area[0].y_pos + 28 < dengray.rows)
			  y_limit = char_area[0].y_pos + 28;
		  else
			  y_limit = dengray.rows;

		  if (char_area[0].x_pos + 196 < dengray.cols)
			  x_limit = char_area[0].x_pos + 196;
		  else
			  x_limit = dengray.cols;

		  if (char_area[0].y_pos - 1 < 0)
			  y_st_limit = 0;
		  else
			  y_st_limit = char_area[0].y_pos - 1;

		  if (char_area[0].x_pos - 1 < 0)
			  x_st_limit = 0;
		  else
			  x_st_limit = char_area[0].x_pos - 1;

		  Range rows(y_st_limit, y_limit);
		  Range cols(x_st_limit, x_limit);
		  //------ EXTRACT 冠字號
		  focus_img = dengray(rows, cols);
		  imwrite("focus.bmp", focus_img);

		  adaptiveThreshold(focus_img, thr_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);
		  imwrite("focus_apt.bmp", thr_img);

		  thr_img.copyTo(merge_img(Rect(x_st_limit, y_st_limit, thr_img.cols, thr_img.rows)));
		  imwrite("merged.bmp", merge_img);
	  }
	  else
	  {
		  adaptiveThreshold(dengray, merge_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);
		  imwrite("merged.bmp", merge_img);

	  }

	  //=============== merged images test
	  //Mat merge_img = Mat::zeros(dengray.size(), CV_8UC1);
	  //Mat merge_img = dengray.clone();
	  // Create an all white mask
	  //Mat src_mask = 255 * Mat::ones(thr_img.rows, thr_img.cols, CV_8UC1);
	  // The location of the center of the src in the dst
	  //Point center(char_area[0].x_pos+10, char_area[0].y_pos+10);
	 // Point center(34, 120);
	  //Mat normal_clone;

	 // thr_img.copyTo(merge_img(Rect(x_st_limit, y_st_limit, thr_img.cols, thr_img.rows)));

	  //seamlessClone(thr_img, merge_img, src_mask, center, normal_clone, NORMAL_CLONE);

	 // imwrite("merged.bmp", merge_img);

	  //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	  //--- 20191219 test
	  //threshold(dengray, thr_img, 150, 255, THRESH_BINARY_INV);

	  //--- good
	  //threshold(dengray, thr_img, 100, 255, THRESH_OTSU+THRESH_BINARY_INV);

	  //- adaptive
	  //adaptiveThreshold(dengray, thr_img, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 51, 20);
          //---- 20200325 adaptive
	  //--- golden
	  //---20200407
	 // adaptiveThreshold(dengray, thr_img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 31, 30);

	  //--- 20200330
	  //threshold(dengray, thr_img, 100, 255, THRESH_BINARY_INV);


	  //imshow("thr window", thr_img);
	 // imwrite("thr.bmp", thr_img);

	  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	  //-------2019/12/30
	  //--- MORPH_RECT, MORPH_CROSS
	 // Mat element = getStructuringElement(MORPH_RECT, Size(2, 1), Point(-1, -1));
	  Mat out_dilate;
	  //Mat out_erode;
	  //進行膨脹操作
	  //dilate(thr_img, out_dilate, element, Point(-1, -1), 1);
	  //--縮點
	  //erode(thr_img, out_erode, element, Point(-1, -1), 1);


	 // imwrite("out_erode.bmp", out_erode);
	  //imshow("erosion demo", out_erode);
	  //---- 2020/1/14 再做 dilation
	  //進行膨脹操作

	 //keep golden: Mat element2 = getStructuringElement(MORPH_RECT, Size(1, 2), Point(-1, -1));
	  Mat element2 = getStructuringElement(MORPH_RECT, Size(1, 2), Point(-1, -1));
	 //-- 1st: Mat element2 = getStructuringElement(MORPH_RECT, Size(2, 2), Point(-1, -1));
	 //---2nd: Mat element2 = getStructuringElement(MORPH_CROSS, Size(2, 2), Point(-1, -1));
	 //-- 3rd: Mat element2 = getStructuringElement(MORPH_CROSS, Size(2, 3), Point(-1, -1));
	 //-- 4th: Mat element2 = getStructuringElement(MORPH_CROSS, Size(1, 3), Point(-1, -1));
	  //Mat element2 = getStructuringElement(MORPH_RECT, Size(1, 3), Point(-1, -1));

	  //dilate(thr_img, out_dilate, element2, Point(-1, -1), 1);
	  dilate(merge_img, out_dilate, element2, Point(-1, -1), 1);



	  //顯示效果圖
	  imwrite("out_dilate.bmp", out_dilate);
	  // imshow("dilation demo", out_dilate);
	  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


	  //------ inv binary image
	  //Mat invbw2_img;
	  //threshold(thr_img, invbw2_img, 128.0, 255.0, THRESH_BINARY_INV);
	  //invbw2_img = thr_img;


	  //---- 2017/12/17 測
	  //threshold(dengray, invbw2_img, 150.0, 255.0, THRESH_BINARY_INV);
	  //imwrite("invbw.bmp", invbw2_img);

#ifdef debug_20191219

	  //string code;
	 // code = pytesseract.image_to_string(invbw2_img, config = 'digits');
	 // cout << "code:" << code << endl;

	  //Mat denoise_img;
      //fastNlMeansDenoisingColored(thr_img, denoise_img, 10, 10, 7, 21);
	  //imwrite("denoise.bmp", denoise_img);

#endif



	  //------- 2019/11/7
	  //---- label FONT
	  //int threshval = 100;

	 // Mat bw = threshval < 128 ? (thr_img < threshval) : (thr_img > threshval);

	 // Mat bw = (thr_img > 60);
	 // Mat bw = (out_erode > 60);
      /*---------20200324
	  Mat bw = (out_dilate > 60);
	  ------- end 20200324 --*/

   /*-----20200323
	  Mat labelImage(thr_img.size(), CV_32S);
	  int nLabels = connectedComponents(bw, labelImage, 8);

	  vector<Vec3b> colors(nLabels);
	  colors[0] = Vec3b(0, 0, 0);//background

	  for (int label = 1; label < nLabels; ++label) {
		  colors[label] = Vec3b((rand() & 255), (rand() & 255), (rand() & 255));
	  }
	  Mat dst(thr_img.size(), CV_8UC3);
	  for (int r = 0; r < dst.rows; ++r) {
		  for (int c = 0; c < dst.cols; ++c) {
			  int label = labelImage.at<int>(r, c);
			  Vec3b &pixel = dst.at<Vec3b>(r, c);
			  pixel = colors[label];
		  }
	  }
	   //imshow("Connected Components", dst);
	 // 0206 imwrite("dst.bmp", dst);
	 // 0206 imwrite("bw_org.bmp", bw);

   ---- 20200323  */

	  //------ 2019/12/19 除雜點
	  //Mat labels(thr_img.size(), CV_32S);
	  Mat labels(merge_img.size(), CV_32S);
	  Mat img_color, stats, centroids;
	  /*--- 20200324
	  int nccomps = connectedComponentsWithStats(
		  bw, labels,
		  stats, centroids
	  );     ----- */
	  int nccomps = connectedComponentsWithStats(
		  out_dilate, labels,
		  stats, centroids
	  );


	  // 0206 cout << "Total Connected Components Detected: " << nccomps << endl;

	  /*-------- 20200324 begin---
	  vector<Vec3b> colors2(nccomps + 1);

	  colors2[0] = Vec3b(0, 0, 0); // background pixels remain black.
	  for (int i = 1; i < nccomps; i++) {
		  //colors2[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		  colors2[i] = Vec3b(255, 255, 255);

		  if (stats.at<int>(i, CC_STAT_AREA) < 40)
			  colors2[i] = Vec3b(0, 0, 0); // small regions are painted with black too.
	  }

	  img_color = Mat::zeros(thr_img.size(), CV_8UC3);

	  for (int y = 0; y < img_color.rows; y++)
		  for (int x = 0; x < img_color.cols; x++)
		  {
			  int label = labels.at<int>(y, x);
			  CV_Assert(0 <= label && label <= nccomps);
			  img_color.at<Vec3b>(y, x) = colors2[label];
		  }

	 // 0206  imwrite("proc.bmp", img_color);

	  ---- 20200324 end ---- */

	  vector<uchar> colors2(nccomps + 1);

	  colors2[0] = 0; // background pixels remain black.
	  for (int i = 1; i < nccomps; i++) {
		 colors2[i] = 255;

		  if (stats.at<int>(i, CC_STAT_AREA) < 40)
			  colors2[i] = 0; // small regions are painted with black too.
	  }

	  Mat denoise_img;
	  denoise_img = Mat::zeros(merge_img.size(), CV_8UC1);

	  for (int y = 0; y < denoise_img.rows; y++)
		  for (int x = 0; x < denoise_img.cols; x++)
		  {
			  int label = labels.at<int>(y, x);
			 // CV_Assert(0 <= label && label <= nccomps);
			  denoise_img.at<uchar>(y, x) = colors2[label];
		  }

	  //--- 20200422
	  imwrite("denoise.bmp", denoise_img);

	  //----20200422 dilation again
	  Mat invbw_img;
	  invbw_img = Mat::zeros(merge_img.size(), CV_8UC1);

	 Mat element3 = getStructuringElement(MORPH_CROSS, Size(1, 3), Point(-1, -1));
	 dilate(denoise_img, invbw_img, element3, Point(-1, -1), 1);



	  //--- 20200326 debug img_section1
	  imwrite("invbw.bmp", invbw_img);

	   //------ 2019/11/21 除雜點= fail
	   /*
	   Mat rem_noise;
	   bilateralFilter(invbw2_img, rem_noise, 5, 21, 21);
	   imwrite("remnoise.bmp", rem_noise);
	   */
	   //-- do check
	   //imwrite("checkremove.bmp", invbw2_img);

	   //----- get each character
	   //---- 2019/11/13
	  vector<vector<Point> > contours;
	  vector<Vec4i> hierarchy;

	 // RNG rng;

	  //---- 2019/12/19 add test
	  /*-- 20200324
	  Mat invbw_img;
	  //invbw_img = img_color;
	  cvtColor(img_color, invbw_img, COLOR_BGR2GRAY);
      ---- 20200324 end*/

	  //-- euler number
	  findContours(invbw_img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
	  //--- below = ok
	  //findContours(invbw_img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

     /*---- 20200323
	  /// Draw contours
	  Mat drawing = Mat::zeros(invbw_img.size(), CV_8UC3);

	  for (int i = 0; i < contours.size(); i++)
	  {
		  Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		  drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
	  }

	  /// Show in a window
	  // 0206 namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	  // 0206 imshow("Contours", drawing);
	  // 0206 imwrite("drawing.bmp", drawing);
     ---20200323---*/

	  //----- input 冠字號
	  Rect  bounding_rect;
	  Mat font_img, font2_img, font3_img;


	  //----do HOG
	  vector<float> ders;
	 // vector<Point> locs;
	 // vector<vector<float> > decideHOG;
	  //-- org font
	 // vector<float> org_ders;
	 // vector<vector<float> > org_HOG;

	   //--- use opencv_hog
     /*  2020/03/06 keep
	  HOGDescriptor hog(
		  Size(64, 64), //winSize
		  Size(16, 16), //blocksize = typical 2x cellsize
		  Size(16, 16), //blockStride,
		  Size(8, 8), //cellSize,
		  9, //nbins,
		  1, //derivAper,
		  -1, //winSigma,
		  0, //histogramNormType,
		  0.2, //L2HysThresh,
		  0,//gammal correction,
		  64,//nlevels=64
		  1);    */

     HOGDescriptor hog(
		  Size(16, 16), //winSize
		  Size(16, 16), //blocksize = typical 2x cellsize
		  Size(16, 16), //blockStride,
		  Size(8, 8), //cellSize,
		  9, //nbins,
		  1, //derivAper,
		  -1, //winSigma,
		  0, //histogramNormType,
		  0.2, //L2HysThresh,
		  0,//gammal correction,
		  64,//nlevels=64
		  1);


#if 0
	  struct svm_parameter
	  {
		  int svm_type;
		  int kernel_type;
		  int degree;	/* for poly */
		  double gamma;	/* for poly/rbf/sigmoid */
		  double coef0;	/* for poly/sigmoid */

						/* these are for training only */
		  double cache_size; /* in MB */
		  double eps;	/* stopping criteria */
		  double C;	/* for C_SVC, EPSILON_SVR and NU_SVR */
		  int nr_weight;		/* for C_SVC */
		  int *weight_label;	/* for C_SVC */
		  double* weight;		/* for C_SVC */
		  double nu;	/* for NU_SVC, ONE_CLASS, and NU_SVR */
		  double p;	/* for EPSILON_SVR */
		  int shrinking;	/* use the shrinking heuristics */
		  int probability; /* do probability estimates */
	  };
#endif
	  /*
	  // default values
	  param.svm_type = C_SVC;
	  param.kernel_type = RBF;
	  param.degree = 3;
	  param.gamma = 0;	// 1/num_features
	  param.coef0 = 0;
	  param.nu = 0.5;
	  param.cache_size = 100;
	  param.C = 1;
	  param.eps = 1e-3;
	  param.p = 0.1;
	  param.shrinking = 1;
	  param.probability = 0;
	  param.nr_weight = 0;
	  param.weight_label = NULL;
	  param.weight = NULL;
	  */

	  //enum { C_SVC, NU_SVC, ONE_CLASS, EPSILON_SVR, NU_SVR };	/* svm_type */
	  //enum { LINEAR, POLY, RBF, SIGMOID, PRECOMPUTED }; /* kernel_type */

	  //--- NTU libsvm
	  /*
	  struct svm_parameter param =
	  {
		  C_SVC,
		  RBF,
		  3,
		  0.027,
		  0,
		  100,
		  1e-3,
		  10,
		  0,
		  NULL,
		  NULL,
		  0.5,
		  0.1,
		  1,
		  0
	  };
	  */





	  //double predict_label, predict_label2;
	  double predict_label;
	  double *prob_estimates = NULL;
	  int center_pix;
	  int left_cor;
	  vector <font_order> final_out;
	  font_order img_pos;


	  //==================================================================

	  //svm_node* x_space_t3 = new svm_node[orghog_descr_size + 1];//temp example feature vector
	  feature_node* x_space_t3 = new feature_node[orghog_descr_size + 1];//temp example feature vector

	  //-----handle current input image
	 //---- 2019/11/27
	  for (int i = 0; i < contours.size(); i++)
	  {

		  //--- 2019/11/18 draw test
		  //  Find the area of contour
		  double a = contourArea(contours[i], false);
		  // double a = countNonZero(contours[i]);
		  // double a = contours[i].size();  //--good
		  int parent = hierarchy[i][3]; // parent

		  //--- debug region
		  bounding_rect = boundingRect(contours[i]);
		  // 0206 cout << "-------------------------" << endl;
		  // 0206 cout << "area = " << a << endl;
		  //cout << "2nd area calculation= " << contourArea(contours[i], false) << endl;
		  // 0206 cout << "  x=" << bounding_rect.x <<" y=" << bounding_rect.y << endl;


		  //-- debug euler num
		  // 0206 printf("i = %d, next %d, previous %d, children : %d, parent : %d\n",
			//   i, hierarchy[i][0], hierarchy[i][1], hierarchy[i][2], hierarchy[i][3]);


		  //-- end of debug region

	  //----- 以area + 第一層為主判斷 + y-pos 條件
     //--- 20200318 residue part removed condition :
		  //---     (a) font top-y value
		  //---     (b) font width value
		  //----    (c) font height value
		//  if (a > 30 && parent == -1) {
		  if (a > 20 && parent == -1  && bounding_rect.y <50
		       && bounding_rect.width <27 && bounding_rect.height >10) {
			  //	  if (a > 14 && parent == -1) {
			  //--- 20200326 delete dummy cmd (boundingrec )
			  //bounding_rect = boundingRect(contours[i]);

			  //---- perf-mark_s
			  //rectangle(denimg, bounding_rect, Scalar(255, 0, 0), 2, 8, 0);
			  //---2019/1209
			 // 0206 cout << "accept font======" << endl;
			 // 0206 cout << bounding_rect << endl;
			  img_pos.x_pos = bounding_rect.x;
			  //---- perf-mark_e

			  //==================================================
			  //------ euler number
#if 1
			  int euler_num;
			  int next = hierarchy[i][0]; // next at the same hierarchical level
			  int prev = hierarchy[i][1]; // prev at the same hierarchical level
			  int child = hierarchy[i][2]; // first child
			  //int  parent = hierarchy[i][3]; // parent
			 // 0206  printf("i = %d, next %d, previous %d, children : %d, parent : %d\n", i, next, prev, child, parent);

			  // start calculate euler number
			  int h_total = 0;
			  int n_total = 1;
			  int index = 1;
			  vector<int> all_children;

			  if (child >= 0 && parent < 0) {
				  // 計算當前層
				  queue<int> nodes;
				  vector<int> indexes = current_layer_holes(hierarchy, child);
				  for (int k = 0; k < indexes.size(); k++) {
					  nodes.push(indexes[k]);
				  }
				  while (!nodes.empty()) {
					  // 當前層總數目
					  if (index % 2 == 0) { // 聯通元件物件
						  n_total += nodes.size();
					  }
					  else { // 孔洞對象
						  h_total += nodes.size();
					  }
					  index++;
					  // 計算下一層所有孩子節點
					  int curr_ndoes = nodes.size();
					  for (int n = 0; n < curr_ndoes; n++) {
						  int value = nodes.front();
						  nodes.pop();
						  // 獲取下一層節點第一個孩子
						  int child = hierarchy[value][2];
						  if (child >= 0) {
							  nodes.push(child);
						  }
					  }
				  }
				  // 0206 printf("hole number : %d\n", h_total);
				  // 0206 printf("connection number : %d\n", n_total);
				  // 計算歐拉數
				  euler_num = n_total - h_total;
				  // 0206 printf("number of euler : %d \n", euler_num);
				  // drawContours(invbw_img, contours, i, Scalar(0, 0, 255), 2, 8);
				   // 顯示歐拉數
				  Rect rect = boundingRect(contours[i]);
				  // putText(invbw_img, format("euler: %d", euler_num), rect.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255, 255, 0), 2, 8);
			  }
			  if (child < 0 && parent < 0) {
				 // 0206 printf("hole number : %d\n", h_total);
				 // 0206 printf("connection number : %d\n", n_total);
				  euler_num = n_total - h_total;
				// 0206  printf("number of euler : %d \n", euler_num);
				  // drawContours(invbw_img, contours, i, Scalar(255, 0, 0), 2, 8);
				  Rect rect = boundingRect(contours[i]);
				  // putText(invbw_img, format("euler: %d", euler_num), rect.tl(), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255, 255, 0), 2, 8);
			  }
#endif
			  //----- END euler number
			  //====================================================


			  //----- debug 20200337 checkafter findcontours image
			  //imwrite("invbw_aft.bmp", invbw_img);

			  //------ cut each input font
			  //------ 2020/1/7 dilation image to 鑑別
			  font_img = invbw_img(bounding_rect);
			  //font_img = out_dilate(bounding_rect);
			  //----- 2020/1/14 mask
			  //font_img = out_erode(bounding_rect);
			 // font_img = out_dilate(bounding_rect);

			  //---resize img
			  resize(font_img, font2_img, Size(16, 32), 0, 0, INTER_LINEAR);//重新調整影像大小
			  //resize(font_img, font2_img, Size(64, 112), 0, 0, INTER_LINEAR);//重新調整影像大小
			  threshold(font2_img, font3_img, 128.0, 255.0, THRESH_BINARY_INV);

#ifdef debug_20191219
			  //---- perf-mark_s
			sprintf(basefilename, dest, i);
    		imwrite(basefilename, font3_img);
			//---test
			//sprintf(basefilename, dest, i+100);
			//imwrite(basefilename, font2_img);
			  //---- perf-mark_e
#endif

			 //----- 2019/12/20
			 //------ add font gravity
			  //--- 2020/3/11 : adjust size value
#if 1
			 float s = 0.0f;
			 uchar* ptr;
			// unsigned char *ptr;

			//for (int row = 70; row<91; row++)
			//--20200427 for (int row = 20; row<26; row++)
			for (int row = 19; row<24; row++)
			 {
				//printf("row = %d\n", row);

				 ptr = font3_img.ptr<uchar>(row);  ///--ok2
				// ptr = (font3_img.data + row*font3_img.step); //-- ok3


				 //for (int col = 22; col<43; col++) {
				  // --20200427 for (int col = 5; col<11; col++) {
				 for (int col = 7; col<11; col++) {
					//cout << (int)*(ptr+col) <<"\t";  //= ok2
                  //cout << (int)font3_img.at<uchar>(row, col) << "\t";   ///--ok

				  //if ((int)font3_img.at<uchar>(row, col) == 0)   //-- ok
					if ((int)*(ptr + col) == 0)  //-- ok2
					 {
						 //s += (int)*(ptr + col);
						 s++;
					 }
				 }
				// printf("sum=%3.1f\n", s);
			 }

#endif

			//----- 2019/12/24 add center_pix feature
			if (s > 0)
				center_pix = 1;
			else
				center_pix = 0;


			  //--- FOR CHECK IMAGE
			  //imshow("CHECK  ", font3_img);

			//---- add "0" + "D" left_corner

			/*----font size =[64,112]
			if ((int)font3_img.at<uchar>(0, 5) == 0 || (int)font3_img.at<uchar>(0, 6) == 0 ||
				(int)font3_img.at<uchar>(111, 5) == 0 || (int)font3_img.at<uchar>(111, 6) == 0 )
				left_cor = 1;
			else
				left_cor = 0;
               */
			//-- font size =[16,32]
			if ((int)font3_img.at<uchar>(0, 0) == 0 || (int)font3_img.at<uchar>(0, 1) == 0 ||
				(int)font3_img.at<uchar>(31, 0) == 0 || (int)font3_img.at<uchar>(31, 1) == 0)
				left_cor = 1;
			else
				left_cor = 0;


        /*   0206
			cout << "left_cor1 =" << (int)font3_img.at<uchar>(0, 5) << endl;
			cout << "left_cor2 =" << (int)font3_img.at<uchar>(0, 6) << endl;
			cout << "left_cor3 =" << (int)font3_img.at<uchar>(111, 5) << endl;
			cout << "left_cor4 =" << (int)font3_img.at<uchar>(111, 6) << endl;
 			cout << "left_cor =" << left_cor << endl;     */


			  //---- do extractHOG
			  hog.compute(font3_img, ders);

			  //====== START
			  //-------- NTU_SVM method
			  for (int z = 0; z < orghog_descr_size; z++) {

				  x_space_t3[z].index = z + 1;
				  x_space_t3[z].value = ders[z];

				  //---- perf-mark_s
				 // file3 << ders[z] << " ";
				 //---- perf-mark_e
			  }
			  //--- add end element
			  x_space_t3[orghog_descr_size].index = -1;
			  x_space_t3[orghog_descr_size].value = 0;


			  //--- check input vector+SVM model SV
			  //---- perf-mark_s
			  //file3 << endl;
			  //---- perf-mark_e

			 // predict_label = svm_predict(model_ntu, x_space_t3);
			   predict_label = predict(model_ntu, x_space_t3);

			 // 0206 cout << "class=" << predict_label << endl;
			  //---- add probability
			  //--- 20191206
			  /*
			  int nr_class = svm_get_nr_class(model_ntu);
			  prob_estimates = (double *)malloc(nr_class * sizeof(double));

			  predict_label2 = svm_predict_probability(model_ntu, x_space_t3, prob_estimates);
			  cout << "prob_class=" << predict_label2 << endl;

			  for (int j = 0; j < nr_class; j++)
				  cout << prob_estimates[j] << " ";
			  cout << "\n";
			  //---- add probability
			  //---END 20191206
			 */
			 //======== END NTU_SVM

		   //---- opencv svm
		   //---- perf-mark_s
		  // decideHOG.push_back(ders);
		  //---- perf-mark_e


			  //-- euler number input
			  img_pos.eulernum = euler_num;
			  //  img_pos.eulernum = 1;
			 //--- collect font order information
			  img_pos.decide_val = predict_label;
			  img_pos.center_wk = center_pix;
			  //-------- 2020/1/17 add "0"+"D" decision condition
			  img_pos.left_cor = left_cor;

			  //------ 2020/4/15
			  img_pos.imgnum = i;

			  final_out.push_back(img_pos);


		  }  //-- end area



	  }


	  //---- 重整 font order

	  sort(final_out.begin(), final_out.end(), struct_cmp_by_x_pos);


	  int decide_out;
	  int euler_ref;
	  int cen_wk;
	  int left_info;
	  //---- 20200420
	  int font_ord;
	  char * cur_note;
	  //char rtn_ocr[10] = { 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };
	  char rtn_ocr[] = "AB123456CD";
	  //string cur_note;

	  string answer;
	  golden_in >> answer;
	  int book_font;
	  //imgnum_in >> book_font;


	  //---- 20200327 shortage of ocr_fonts
	  if (final_out.size() == 10)
	  {    //---- normal flow start

		  for (int n = 0; n < 10; n++)
		  {

			  decide_out = final_out[n].decide_val;
			  euler_ref = final_out[n].eulernum;
			  cen_wk = final_out[n].center_wk;
			  //-- 2020/0117
			  left_info = final_out[n].left_cor;
			  font_ord = final_out[n].imgnum;
			  //---20200423 read be-stored num
			  imgnum_in >> book_font;

			  //----- alphabet
			  if ((n < 2) || (n > 7))
			  {

				  switch (decide_out) {
				  case 1:
					  // 0206 cout << "A";
					  rpt_out << "A";
					  rpt_out2 << setw(3) << 1;
					  cur_note = "A";
					  rtn_ocr[n] = 65;   //--ascii
					  break;
				  case 2:
					  // 0206 cout << "O";
					  if (euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  else {
						  rpt_out << "O";
						  rpt_out2 << setw(3) << 2;
						  cur_note = "O";
						  rtn_ocr[n] = 79;   //--ascii
					  }
					  break;
				  case 3:
					  if (euler_ref == -1) {
						  // 0206 cout << "B";      //-- org
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note =  "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note =  "P";
						  rtn_ocr[n] = 80;   //--ascii

					  }
					  break;
				  case 4:
					  if (euler_ref == 0) {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note =  "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  break;
				  case 5:
					  if (euler_ref == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "C";   //-- org
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  break;
				  case 6:
					  if (cen_wk == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else if (euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "Q";     //-- org
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  break;
				  case 7:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  else if (euler_ref == 0) {
						  // 0206 cout << "D";  //--- org
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "C";
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  break;
				  case 8:
					  // 0206 cout << "R";
					  rpt_out << "R";
					  rpt_out2 << setw(3) << 8;
					  cur_note = "R";
					  rtn_ocr[n] = 82;   //--ascii
					  break;
				  case 9:
					  if (euler_ref == 1) {
						  // 0206 cout << "E";     //-- org
						  rpt_out << "E";
						  rpt_out2 << setw(3) << 9;
						  cur_note = "E";
						  rtn_ocr[n] = 69;   //--ascii
					  }
					  else if (euler_ref == -1)
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  break;
				  case 10:
					  // 0206 cout << "S";
					  rpt_out << "S";
					  rpt_out2 << setw(3) << 10;
					  cur_note = "S";
					  rtn_ocr[n] = 83;   //--ascii
					  break;
				  case 11:
					  // 0206 cout << "F";
					  if (euler_ref == 0) {
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  else
					  {
						  rpt_out << "F";
						  rpt_out2 << setw(3) << 11;
						  cur_note = "F";
						  rtn_ocr[n] = 70;   //--ascii
					  }
					  break;
				  case 12:
					  // 0206 cout << "T";
					  rpt_out << "T";
					  rpt_out2 << setw(3) << 12;
					  cur_note = "T";
					  rtn_ocr[n] = 84;   //--ascii
					  break;
				  case 13:
					  //--- 2020/3/13 add "B"+ "D"
					  if (euler_ref == -1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else if (euler_ref == 0) {
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "G";
						  rpt_out << "G";       //--- org
						  rpt_out2 << setw(3) << 13;
						  cur_note = "G";
						  rtn_ocr[n] = 71;   //--ascii
					  }
					  break;
				  case 14:
					  if (euler_ref == 1) {
						  // 0206 cout << "U";          //-- org
						  rpt_out << "U";
						  rpt_out2 << setw(3) << 14;
						  cur_note = "U";
						  rtn_ocr[n] = 85;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  break;
				  case 15:
					  if (euler_ref == 1) {
						  // 0206 cout << "H";          //-- org
						  rpt_out << "H";
						  rpt_out2 << setw(3) << 15;
						  cur_note = "H";
						  rtn_ocr[n] = 72;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  break;
				  case 16:
					  // 0206 cout << "V";
					  rpt_out << "V";
					  rpt_out2 << setw(3) << 16;
					  cur_note = "V";
					  rtn_ocr[n] = 86;   //--ascii
					  break;
				  case 17:
					  // 0206 cout << "I";
					  if (euler_ref == -1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else {
						  rpt_out << "I";
						  rpt_out2 << setw(3) << 17;
						  cur_note = "I";
						  rtn_ocr[n] = 73;   //--ascii
					  }
					  break;
				  case 18:
					  // 0206 cout << "W";
					  rpt_out << "W";
					  rpt_out2 << setw(3) << 18;
					  cur_note = "W";
					  rtn_ocr[n] = 87;   //--ascii
					  break;
				  case 19:
					  // 0206 cout << "J";
					  rpt_out << "J";
					  rpt_out2 << setw(3) << 19;
					  cur_note = "J";
					  rtn_ocr[n] = 74;   //--ascii
					  break;
				  case 20:
					  // 0206 cout << "K";
					  rpt_out << "K";
					  rpt_out2 << setw(3) << 20;
					  cur_note = "K";
					  rtn_ocr[n] = 75;   //--ascii
					  break;
				  case 21:
					  // 0206 cout << "X";
					  rpt_out << "X";
					  rpt_out2 << setw(3) << 21;
					  cur_note = "X";
					  rtn_ocr[n] = 88;   //--ascii
					  break;
				  case 22:
					  if (euler_ref == 1) {
						  // 0206 cout << "L";
						  rpt_out << "L";
						  rpt_out2 << setw(3) << 22;
						  cur_note = "L";
						  rtn_ocr[n] = 76;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  break;
				  case 23:
					  // 0206 cout << "Y";
					  rpt_out << "Y";
					  rpt_out2 << setw(3) << 23;
					  cur_note = "Y";
					  rtn_ocr[n] = 89;   //--ascii
					  break;
				  case 24:
					  // 0206 cout << "M";
					  rpt_out << "M";
					  rpt_out2 << setw(3) << 24;
					  cur_note = "M";
					  rtn_ocr[n] = 77;   //--ascii
					  break;
				  case 25:
					  // 0206 cout << "Z";
					  rpt_out << "Z";
					  rpt_out2 << setw(3) << 25;
					  cur_note = "Z";
					  rtn_ocr[n] = 90;   //--ascii
					  break;
				  case 26:
					  // 0206 cout << "N";
					  rpt_out << "N";
					  rpt_out2 << setw(3) << 26;
					  cur_note = "N";
					  rtn_ocr[n] = 78;   //--ascii
					  break;
				  default:
					  //--- add euler number
					  if (decide_out == 27 && euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  //------ 2020/3/12 add "D"
					  else if (decide_out == 27 && euler_ref == 0 && left_info == 1) {
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  //------ 2020/3/13 add "C"
					  else if (decide_out == 27 && euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  //------ 2020/3/13 add "T"
					  else if (decide_out == 34) {
						  rpt_out << "T";
						  rpt_out2 << setw(3) << 12;
						  cur_note = "T";
						  rtn_ocr[n] = 84;   //--ascii
					  }
					  //----- 2020/3/12 separate "B'+"8"
					  else if (decide_out == 35 && euler_ref == -1 && left_info == 1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else if (decide_out == 33 && euler_ref == -1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "$";
						  rpt_out << "$";
						  rpt_out2 << setw(3) << 37;
						  cur_note = "$";
						  rtn_ocr[n] = 36;   //--ascii
					  }
				  } // end of switch
			  }   ///-- end of alphabet
			  else
			  {

				  switch (decide_out) {
				  case 27:
					  // 0206 cout << "0";
					  rpt_out << "0";
					  rpt_out2 << setw(3) << 27;
					  cur_note = "0";
					  rtn_ocr[n] = 48;   //--ascii
					  break;
				  case 28:
					  // 0206 cout << "1";
					  rpt_out << "1";
					  rpt_out2 << setw(3) << 28;
					  cur_note = "1";
					  rtn_ocr[n] = 49;   //--ascii
					  break;
				  case 29:
					  // 0206 cout << "2";
					  rpt_out << "2";
					  rpt_out2 << setw(3) << 29;
					  cur_note = "2";
					  rtn_ocr[n] = 50;   //--ascii
					  break;
				  case 30:
					  if (euler_ref == -1) {
						  rpt_out << "8";
						  rpt_out2 << setw(3) << 35;
						  cur_note = "8";
						  rtn_ocr[n] = 56;   //--ascii
					  }
					  else {
						  // 0206 cout << "3";
						  rpt_out << "3";
						  rpt_out2 << setw(3) << 30;
						  cur_note = "3";
						  rtn_ocr[n] = 51;   //--ascii
					  }
					  break;
				  case 31:
					  // 0206 cout << "4";
					  rpt_out << "4";
					  rpt_out2 << setw(3) << 31;
					  cur_note = "4";
					  rtn_ocr[n] = 52;   //--ascii
					  break;
				  case 32:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "5";  //--org
						  rpt_out << "5";
						  rpt_out2 << setw(3) << 32;
						  cur_note = "5";
						  rtn_ocr[n] = 53;   //--ascii
					  }
					  break;
				  case 33:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";     //-- org
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  else if (euler_ref == -1)
					  {
						  rpt_out << "8";
						  rpt_out2 << setw(3) << 35;
						  cur_note = "8";
						  rtn_ocr[n] = 56;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "5";
						  rpt_out << "5";
						  rpt_out2 << setw(3) << 32;
						  cur_note = "5";
						  rtn_ocr[n] = 53;   //--ascii
					  }
					  break;
				  case 34:
					  // 0206 cout << "7";
					  rpt_out << "7";
					  rpt_out2 << setw(3) << 34;
					  cur_note = "7";
					  rtn_ocr[n] = 55;   //--ascii
					  break;
				  case 35:
					  if (euler_ref == -1) {
						  // 0206 cout << "8";         //-- org
						  rpt_out << "8";
						  rpt_out2 << setw(3) << 35;
						  cur_note = "8";
						  rtn_ocr[n] = 56;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  break;
				  case 36:
					  // 0206 cout << "9";
					  rpt_out << "9";
					  rpt_out2 << setw(3) << 36;
					  cur_note = "9";
					  rtn_ocr[n] = 57;   //--ascii
					  break;
				  default:
					  //--- 2020/3/13 add "0"
				   //  if ((decide_out == 9 || decide_out == 3 || decide_out == 7 )  && euler_ref == 0)
					  if ((decide_out == 9 || decide_out == 3) && euler_ref == 0)
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  else if (decide_out == 7 && left_info == 0)
					  {
						  rpt_out << "0";
						  rpt_out2 << setw(3) << 27;
						  cur_note = "0";
						  rtn_ocr[n] = 48;   //--ascii
					  }
					  else if (decide_out == 3 && euler_ref == -1) {
						  // 0206 cout << "8";
						  rpt_out << "8";
						  rpt_out2 << setw(3) << 35;
						  cur_note = "8";
						  rtn_ocr[n] = 56;   //--ascii
					  }
					  else if (decide_out == 12) {
						  // 0206 cout << "1";
						  rpt_out << "1";
						  rpt_out2 << setw(3) << 28;
						  cur_note = "1";
						  rtn_ocr[n] = 49;   //--ascii
					  }
					  else if (decide_out == 6 && cen_wk == 0) {
						  // 0206 cout << "0";
						  rpt_out << "0";
						  rpt_out2 << setw(3) << 27;
						  cur_note = "0";
						  rtn_ocr[n] = 48;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "$";
						  rpt_out << "$";
						  rpt_out2 << setw(3) << 37;
						  cur_note = "$";
						  rtn_ocr[n] = 36;   //--ascii
					  }
				  } //---end switch

			  }//-- end if

            //-------- catch error font image
			//---- 對答案
			  //------ compare string
			  if (answer[n] != cur_note[0]) {
				 // cout << "answer =" << answer[n] << "  cur_note =" << cur_note[0] << endl;
				  rec_out << "#img=" << k << " pos=" << n << " word=" << answer[n] << " font_num=" << book_font << endl;
				 // rec_out <<"#img=" << k << " pos=" << n << " word=" << answer[n]  << endl;

				  char libfont[100];
				  char *libfont_path = "./input%d_1.bmp";
				  char stofont[100];
				  char *storfont_path = "./font_col_a/st_%d.bmp";
				  sprintf(libfont, libfont_path, font_ord);
				  sprintf(stofont, storfont_path, total_col_img);
				  cout << "org = " << libfont << endl;
				  cout << "dest = " << stofont << endl;
				  total_col_img++;

				  CopyFile(libfont, stofont);

			  }   //-- end compare


		  } //--- end for

	  } //--- normal flow end
	  else
	  { //------2nd ocr flow start
		  for (int n = 0; n < 10; n++)
		  {
			  imgnum_in >> book_font;
		  }


		  for (int n = 0; n < final_out.size(); n++)
		  {

			  decide_out = final_out[n].decide_val;
			  euler_ref = final_out[n].eulernum;
			  cen_wk = final_out[n].center_wk;
			  //-- 2020/0117
			  left_info = final_out[n].left_cor;
			  font_ord = final_out[n].imgnum;

			  //----- alphabet +arabic mixed

				  switch (decide_out) {
				  case 1:
					  // 0206 cout << "A";
					  rpt_out << "A";
					  rpt_out2 << setw(3) << 1;
					  cur_note = "A";
					  rtn_ocr[n] = 65;   //--ascii
					  break;
				  case 2:
					  // 0206 cout << "O";
					  if (euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  else {
						  rpt_out << "O";
						  rpt_out2 << setw(3) << 2;
						  cur_note = "O";
						  rtn_ocr[n] = 79;   //--ascii
					  }
					  break;
				  case 3:
					  if (euler_ref == -1) {
						  // 0206 cout << "B";      //-- org
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  break;
				  case 4:
					  if (euler_ref == 0) {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  break;
				  case 5:
					  if (euler_ref == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "C";   //-- org
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  break;
				  case 6:
					  if (cen_wk == 0) {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else if (euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "Q";     //-- org
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  break;
				  case 7:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  else if (euler_ref == 0) {
						  // 0206 cout << "D";  //--- org
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "C";
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  break;
				  case 8:
					  // 0206 cout << "R";
					  rpt_out << "R";
					  rpt_out2 << setw(3) << 8;
					  cur_note = "R";
					  rtn_ocr[n] = 82;   //--ascii
					  break;
				  case 9:
					  if (euler_ref == 1) {
						  // 0206 cout << "E";     //-- org
						  rpt_out << "E";
						  rpt_out2 << setw(3) << 9;
						  cur_note = "E";
						  rtn_ocr[n] = 69;   //--ascii
					  }
					  else if (euler_ref == -1)
					  {
						  // 0206 cout << "B";
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  break;
				  case 10:
					  // 0206 cout << "S";
					  rpt_out << "S";
					  rpt_out2 << setw(3) << 10;
					  cur_note = "S";
					  rtn_ocr[n] = 83;   //--ascii
					  break;
				  case 11:
					  // 0206 cout << "F";
					  if (euler_ref == 0) {
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  else
					  {
						  rpt_out << "F";
						  rpt_out2 << setw(3) << 11;
						  cur_note = "F";
						  rtn_ocr[n] = 70;   //--ascii
					  }
					  break;
				  case 12:
					  // 0206 cout << "T";
					  rpt_out << "T";
					  rpt_out2 << setw(3) << 12;
					  cur_note = "T";
					  rtn_ocr[n] = 84;   //--ascii
					  break;
				  case 13:
					  //--- 2020/3/13 add "B"+ "D"
					  if (euler_ref == -1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else if (euler_ref == 0) {
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "G";
						  rpt_out << "G";       //--- org
						  rpt_out2 << setw(3) << 13;
						  cur_note = "G";
						  rtn_ocr[n] = 71;   //--ascii
					  }
					  break;
				  case 14:
					  if (euler_ref == 1) {
						  // 0206 cout << "U";          //-- org
						  rpt_out << "U";
						  rpt_out2 << setw(3) << 14;
						  cur_note = "U";
						  rtn_ocr[n] = 85;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  break;
				  case 15:
					  if (euler_ref == 1) {
						  // 0206 cout << "H";          //-- org
						  rpt_out << "H";
						  rpt_out2 << setw(3) << 15;
						  cur_note = "H";
						  rtn_ocr[n] = 72;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "P";
						  rpt_out << "P";
						  rpt_out2 << setw(3) << 4;
						  cur_note = "P";
						  rtn_ocr[n] = 80;   //--ascii
					  }
					  break;
				  case 16:
					  // 0206 cout << "V";
					  rpt_out << "V";
					  rpt_out2 << setw(3) << 16;
					  cur_note = "V";
					  rtn_ocr[n] = 86;   //--ascii
					  break;
				  case 17:
					  // 0206 cout << "I";
					  if (euler_ref == -1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else {
						  rpt_out << "I";
						  rpt_out2 << setw(3) << 17;
						  cur_note = "I";
						  rtn_ocr[n] = 73;   //--ascii
					  }
					  break;
				  case 18:
					  // 0206 cout << "W";
					  rpt_out << "W";
					  rpt_out2 << setw(3) << 18;
					  cur_note = "W";
					  rtn_ocr[n] = 87;   //--ascii
					  break;
				  case 19:
					  // 0206 cout << "J";
					  rpt_out << "J";
					  rpt_out2 << setw(3) << 19;
					  cur_note = "J";
					  rtn_ocr[n] = 74;   //--ascii
					  break;
				  case 20:
					  // 0206 cout << "K";
					  rpt_out << "K";
					  rpt_out2 << setw(3) << 20;
					  cur_note = "K";
					  rtn_ocr[n] = 75;   //--ascii
					  break;
				  case 21:
					  // 0206 cout << "X";
					  rpt_out << "X";
					  rpt_out2 << setw(3) << 21;
					  cur_note = "X";
					  rtn_ocr[n] = 88;   //--ascii
					  break;
				  case 22:
					  if (euler_ref == 1) {
						  // 0206 cout << "L";
						  rpt_out << "L";
						  rpt_out2 << setw(3) << 22;
						  cur_note = "L";
						  rtn_ocr[n] = 76;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "D";
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  break;
				  case 23:
					  // 0206 cout << "Y";
					  rpt_out << "Y";
					  rpt_out2 << setw(3) << 23;
					  cur_note = "Y";
					  rtn_ocr[n] = 89;   //--ascii
					  break;
				  case 24:
					  // 0206 cout << "M";
					  rpt_out << "M";
					  rpt_out2 << setw(3) << 24;
					  cur_note = "M";
					  rtn_ocr[n] = 77;   //--ascii
					  break;
				  case 25:
					  // 0206 cout << "Z";
					  rpt_out << "Z";
					  rpt_out2 << setw(3) << 25;
					  cur_note = "Z";
					  rtn_ocr[n] = 90;   //--ascii
					  break;
				  case 26:
					  // 0206 cout << "N";
					  rpt_out << "N";
					  rpt_out2 << setw(3) << 26;
					  cur_note = "N";
					  rtn_ocr[n] = 78;   //--ascii
					  break;
				  case 27:
					  if (euler_ref == 0 && cen_wk == 1) {
						  // 0206 cout << "Q";
						  rpt_out << "Q";
						  rpt_out2 << setw(3) << 6;
						  cur_note = "Q";
						  rtn_ocr[n] = 81;   //--ascii
					  }
					  //------ 2020/3/12 add "D"
					  else if (euler_ref == 0 && left_info == 1) {
						  rpt_out << "D";
						  rpt_out2 << setw(3) << 7;
						  cur_note = "D";
						  rtn_ocr[n] = 68;   //--ascii
					  }
					  //------ 2020/3/13 add "C"
					  else if ( euler_ref == 1) {
						  rpt_out << "C";
						  rpt_out2 << setw(3) << 5;
						  cur_note = "C";
						  rtn_ocr[n] = 67;   //--ascii
					  }
					  else {
						  // 0206 cout << "0"; //-- normal
						  rpt_out << "0";
						  rpt_out2 << setw(3) << 27;
						  cur_note = "0";
						  rtn_ocr[n] = 48;   //--ascii
					  }
					  break;
				  case 28:
					  // 0206 cout << "1";
					  rpt_out << "1";
					  rpt_out2 << setw(3) << 28;
					  cur_note = "1";
					  rtn_ocr[n] = 49;   //--ascii
					  break;
				  case 29:
					  // 0206 cout << "2";
					  rpt_out << "2";
					  rpt_out2 << setw(3) << 29;
					  cur_note = "2";
					  rtn_ocr[n] = 50;   //--ascii
					  break;
				  case 30:
					  // 0206 cout << "3";
					  rpt_out << "3";
					  rpt_out2 << setw(3) << 30;
					  cur_note = "3";
					  rtn_ocr[n] = 51;   //--ascii
					  break;
				  case 31:
					  // 0206 cout << "4";
					  rpt_out << "4";
					  rpt_out2 << setw(3) << 31;
					  cur_note = "4";
					  rtn_ocr[n] = 52;   //--ascii
					  break;
				  case 32:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "5";  //--org
						  rpt_out << "5";
						  rpt_out2 << setw(3) << 32;
						  cur_note = "5";
						  rtn_ocr[n] = 53;   //--ascii
					  }
					  break;
				  case 33:
					  if (euler_ref == 0) {
						  // 0206 cout << "6";     //-- org
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "5";
						  rpt_out << "5";
						  rpt_out2 << setw(3) << 32;
						  cur_note = "5";
						  rtn_ocr[n] = 53;   //--ascii
					  }
					  break;
				  case 34:
					  // 0206 cout << "7";
					  rpt_out << "7";
					  rpt_out2 << setw(3) << 34;
					  cur_note = "7";
					  rtn_ocr[n] = 55;   //--ascii
					  break;
				  case 35:
					  if (euler_ref == -1 && left_info == 1) {
						  rpt_out << "B";
						  rpt_out2 << setw(3) << 3;
						  cur_note = "B";
						  rtn_ocr[n] = 66;   //--ascii
					  }
					  else if (euler_ref == -1) {
						  // 0206 cout << "8";         //-- org
						  rpt_out << "8";
						  rpt_out2 << setw(3) << 35;
						  cur_note = "8";
						  rtn_ocr[n] = 56;   //--ascii
					  }
					  else
					  {
						  // 0206 cout << "6";
						  rpt_out << "6";
						  rpt_out2 << setw(3) << 33;
						  cur_note = "6";
						  rtn_ocr[n] = 54;   //--ascii
					  }
					  break;
				  case 36:
					  // 0206 cout << "9";
					  rpt_out << "9";
					  rpt_out2 << setw(3) << 36;
					  cur_note = "9";
					  rtn_ocr[n] = 57;   //--ascii
					  break;
				  default:
						  // 0206 cout << "$";
						  rpt_out << "$";
						  rpt_out2 << setw(3) << 37;
						  cur_note = "$";
						  rtn_ocr[n] = 36;   //--ascii
				  } // end of switch

					//-------- catch error font image
					//---- 對答案
					//------ compare string
				  if (answer[n] != cur_note[0]) {
					 // cout << "answer =" << answer[n] << "  cur_note =" << cur_note[0] << endl;
					 // rec_out << " pos=" << n << " word=" << answer[n] << endl;

					  /*--------  超出10個字, 暫不存
					  char libfont[100];
					  char *libfont_path = "G:\\ljf_repos\\OpenCV_test\\test1_lin\\test1\\input%d_1.bmp";
					  char stofont[100];
					  char *storfont_path = "G:\\ljf_repos\\OpenCV_test\\test1_lin\\test1\\font_col_a\\input%d_1.bmp";
					  sprintf(libfont, libfont_path, font_ord);
					  sprintf(stofont, storfont_path, total_col_img);
					  cout << "org = " << libfont << endl;
					  cout << "dest = " << stofont << endl;
					  total_col_img++;

					  CopyFile(libfont, stofont);
					  */
				  }   //-- end compare


		  } //--- end for
	  }    //---- 2nd ocr flow end




	  //--- 換行
	  // 0206 cout << endl;
	  rpt_out << endl;
	  rpt_out2 << endl;

	  //---- 20200429
	  //cout << "pointer word =" << rtn_ocr << endl;
	 //return &rtn_ocr[0];

  } //--- end of group image (img 57-105 )


#ifdef MAKELIB
} // else if( state == 1)
else if( state == 2)
{
#endif

  rpt_out.close();
  rpt_out2.close();

  //---- add close
  imgnum_in.close();
  rec_out.close();
  golden_in.close();

  //------------------------------------------------------------------------
  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  //------結合收集error fonts+ 產生new model

  string rd_sentence;
  string wk_s;
  int skip_ord = 1;
  int st_num = 0;
  int char_num;
  //double *own_list;
  int own_list[37] = { 0 };
  int wtfont_num = 0;

  //----(1) 收集字型

#ifdef tw_sec13
  ifstream coll_in("tw_1080_1213_errorlist.txt");
#endif


  do {

	  coll_in >> rd_sentence;
	  if (skip_ord % 4 == 0)
	  {
		  wk_s = rd_sentence;
		  // cout << wk_s << endl;
       st_num++;

		  //---- convert to integer
		  // char f_num[3] = { '3', '3', '\0' };
		  char f_num[3] = "77";

		  for (int i = 0; i <2; i++) {
			  f_num[i] = wk_s[i + 9];
			  // cout << f_num[i];
		  }

		  //cout << "f_num= " << f_num << endl;
		  char_num = atoi(f_num);
		 // cout << "choose num =" << char_num << endl;
		  //*(own_list + st_num) = char_num;


      //---2020/05/19
       if (own_list[char_num] == 0) {
				 wtfont_num++;
				 own_list[char_num]++;
         //--------- choose image
				 char orgfont[100];
				 char *orgfont_path = "./font_col_a/st_%d.bmp";
				 char addfont[100];
				 char *addfont_path = "./choose_10/M%d.bmp";
				 sprintf(orgfont, orgfont_path, st_num);
				 sprintf(addfont, addfont_path, char_num);
				 cout << "org = " << orgfont << endl;
				 cout << "dest = " << addfont << endl;

				 CopyFile(orgfont, addfont);

			 }
       else if (own_list[char_num] == 1) {
				 wtfont_num++;
				 own_list[char_num]++;

				 //--------- choose image
				 char orgfont[100];
				 char *orgfont_path = "./font_col_a/st_%d.bmp";
				 char addfont[100];
				 char *addfont_path = "./choose_11/M%d.bmp";
				 sprintf(orgfont, orgfont_path, st_num);
				 sprintf(addfont, addfont_path, char_num);
				 cout << "org = " << orgfont << endl;
				 cout << "dest = " << addfont << endl;

				 CopyFile(orgfont, addfont);


			 }
     }
		 skip_ord++;

	 } while (!coll_in.eof());

     //----- copy two special fonts ="I" + "O"
	 //---- choose_10
	 CopyFile("./font_col_a/special_font/M2.bmp",
		 "./choose_10/M2.bmp");
	 CopyFile("./font_col_a/special_font/M17.bmp",
		 "./choose_10/M17.bmp");

	 //--- choose_11
	 CopyFile("./font_col_a/special_font/M2.bmp",
		 "./choose_11/M2.bmp");
	 CopyFile("./font_col_a/special_font/M17.bmp",
		 "./choose_11/M17.bmp");


    //---- add close
    coll_in.close();


   //====================================only build a new model
  //===================================
  //------original golden font
  //--- font lib
  Mat orgfont_img;
  char *libfont = "./choose_%d/M%d.bmp";


  //char *libfont = "G:\\ljf_repos\\OpenCV_test\\test1\\test1\\fontlib5\\M%d.bmp";
  //char *libfont2 = "G:\\ljf_repos\\OpenCV_test\\test1\\test1\\fontlib6\\M%d.bmp";
  //char *libfont3 = "G:\\ljf_repos\\OpenCV_test\\test1\\test1\\fontlib3\\M%d.bmp";
  //char *libfont4 = "G:\\ljf_repos\\OpenCV_test\\test1\\test1\\fontlib4\\M%d.bmp";
 // char fontname[100] = " ";



  //struct model* model_ntu;


  //-- org font
  vector<float> org_ders;
  vector<vector<float> > org_HOG;


  HOGDescriptor hog(
	  Size(16, 16), //winSize
	  Size(16, 16), //blocksize
	  Size(16, 16), //blockStride,
	  Size(8, 8), //cellSize,
	  9, //nbins,
	  1, //derivAper,
	  -1, //winSigma,
	  0, //histogramNormType,
	  0.2, //L2HysThresh,
	  0,//gammal correction,
	  64,//nlevels=64
	  1);

  //-----read original two font_libs
  for (int j = 10; j < 12; j++) {
	  //------- 2019/11/26
	  //----- 建立冠字號font feature SVM
	  for (int i = 1; i < 37; i++)
	  {
		  sprintf(fontname, libfont, j, i);
		  orgfont_img = imread(fontname);

		 // cout << fontname << endl;

		  hog.compute(orgfont_img, org_ders);
		  //-- check
		  //
		  org_HOG.push_back(org_ders);

	  }
  }

  //----prepare features mat to build model
  //int orghog_descr_size = org_HOG[0].size();
  // cout << "orghog_descr_size  : " << orghog_descr_size << endl;


  //----prepare features mat to build model
  //int orghog_descr_size = org_HOG[0].size();
  int vector_num = org_HOG.size();
  cout << "orghog_descr_size  : " << orghog_descr_size << endl;
  cout << "input vector num  : " << vector_num << endl;


#if 0
  struct parameter
  {
	  int solver_type;

	  /* these are for training only */
	  double eps;             /* stopping criteria */
	  double C;
	  int nr_weight;
	  int *weight_label;
	  double* weight;
	  double p;
  };

  for classification
	  s = 0, L2R_LR                L2 - regularized logistic regression(primal)
	  L2R_L2LOSS_SVC_DUAL   L2 - regularized L2 - loss support vector classification(dual)
	  s = 2, L2R_L2LOSS_SVC        L2 - regularized L2 - loss support vector classification(primal)
	  L2R_L1LOSS_SVC_DUAL   L2 - regularized L1 - loss support vector classification(dual)
	  s = 4, MCSVM_CS              support vector classification by Crammer and Singer
	  L1R_L2LOSS_SVC        L1 - regularized L2 - loss support vector classification
	  s6, L1R_LR                L1 - regularized logistic regression
	  L2R_LR_DUAL           L2 - regularized logistic regression(dual)
	  for regression
		  L2R_L2LOSS_SVR        L2 - regularized L2 - loss support vector regression(primal)
		  L2R_L2LOSS_SVR_DUAL   L2 - regularized L2 - loss support vector regression(dual)
		  L2R_L1LOSS_SVR_DUAL   L2 - regularized L1 - loss support vector regression(dual)

#endif

		  struct parameter param =
	  {
		  // L2R_L2LOSS_SVC,
		  // 0.01,
		  L2R_LR,
		  0.01,
		  // MCSVM_CS,
		  //0.1,
		  // L1R_LR,
		  //0.01,
		  1,
		  0,
		  NULL,
		  NULL,
		  0.1,
		  NULL
	  };



  //struct svm_problem prob;
  struct problem prob;

  prob.l = vector_num; //number of training examples

					   //--- 20200319 add "n" feature_vector elements
  prob.n = orghog_descr_size;

  double * tmp_y = new double[prob.l]; //--- total

  for (int z = 0; z < 2; z++) {
	  for (int s = 0; s < 36; s++)
	  {
		  tmp_y[z * 36 + s] = s + 1;

	  }
  }


 // for (int z = 0; z < prob.l; z++) {
	//  cout << tmp_y[z] << endl;
//  }


  prob.y = tmp_y;


  //--- write HOG feature file
  fstream file2;
  file2.open("hog_feature.txt", ios::out);      //開啟檔案
												//--- write HOG feature file
												//fstream file3, file4;
												// file3.open("input_feature.txt", ios::out);      //開啟檔案
												//file4.open("ntumodel_feature.txt", ios::out);      //開啟檔案

												//--- 20200319 --mask
												//svm_node** x = new svm_node *[prob.l];//Array of pointers to pointers to arrays

  feature_node** x = new feature_node *[prob.l];

  for (int n = 0; n < prob.l; n++) {

	  feature_node* x_space_t = new feature_node[orghog_descr_size + 1];//temp example feature vector

																		//---20200206  sample number
	  file2 << tmp_y[n] << " ";


	  for (int z = 0; z < orghog_descr_size; z++) {

		  x_space_t[z].index = z + 1;
		  x_space_t[z].value = org_HOG[n][z];

		  //---- 20200206
		  //--- vector element number
		  file2 << z + 1 << ":";
		  file2 << org_HOG[n][z] << " ";
	  }
	  file2 << endl;

	  //--- add vector_end element index=-1
	  x_space_t[orghog_descr_size].index = -1;
	  x_space_t[orghog_descr_size].value = 0;


	  //----- assign
	  x[n] = x_space_t;
  }


  prob.x = x;//Assign x to the struct field prob.x

			 //-- 20200319 add
  prob.bias = -1.0;

  //2019/11/29------ do svmtrain
  //--- 2019/12/04 ok
  // model_ntu = svm_train(&prob, &param);
  model_ntu = train(&prob, &param);

  //------- 2019/12/17 save svm model
  //int svm_save_model(const char *model_file_name,
  //			       const struct svm_model *model);

  char *model_file_name = "cv_tw_model2.txt";

  //svm_save_model(model_file_name, model_ntu);
  save_model(model_file_name, model_ntu);


  file2.close();

 // system("pause");

 // cvWaitKey(0);
  //destroyAllWindows();
 // return 0;


#ifdef MAKELIB
    }
#endif
}







