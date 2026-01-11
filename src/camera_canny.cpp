/**
*/
#include <ctime>
#include <iostream>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "canny_util.h"
#include <unistd.h>

using namespace std;
using namespace cv;

/* Possible options: 320x240, 640x480, 1024x768, 1280x1040, and so on. */
/* Pi Camera MAX resolution: 2592x1944 */

#define WIDTH 640
#define HEIGHT 480
#define NFRAME 1.0

int main(int argc, char **argv)
{
   char* dirfilename;        /* Name of the output gradient direction image */
   char outfilename[128];    /* Name of the output "edge" image */
   char composedfname[128];  /* Name of the output "direction" image */
   unsigned char *image;     /* The input image */
   unsigned char *edge;      /* The output edge image */
   int rows, cols;           /* The dimensions of the image. */
   float sigma,              /* Standard deviation of the gaussian kernel. */
	 tlow,               /* Fraction of the high threshold in hysteresis. */
	 thigh;              /* High hysteresis threshold control. The actual
			        threshold is the (100 * thigh) percentage point
			        in the histogram of the magnitude of the
			        gradient image that passes non-maximal
			        suppression. */
   float cap_len {0.0};//time of capturing in 100 million seconds
   /****************************************************************************
   * Get the command line arguments.
   ****************************************************************************/
   if(argc < 5){
   fprintf(stderr,"\n<USAGE> %s sigma tlow thigh [writedirim]\n",argv[0]);
      fprintf(stderr,"      sigma:      Standard deviation of the gaussian");
      fprintf(stderr," blur kernel.\n");
      fprintf(stderr,"      tlow:       Fraction (0.0-1.0) of the high ");
      fprintf(stderr,"edge strength threshold.\n");
      fprintf(stderr,"      thigh:      Fraction (0.0-1.0) of the distribution");
      fprintf(stderr," of non-zero edge\n                  strengths for ");
      fprintf(stderr,"hysteresis. The fraction is used to compute\n");
      fprintf(stderr,"                  the high edge strength threshold.\n");
      fprintf(stderr,"      writedirim: Optional argument to output ");
      fprintf(stderr,"a floating point");
      fprintf(stderr," direction image.\n\n");
      exit(1);
   }

   sigma = atof(argv[1]);
   tlow = atof(argv[2]);
   thigh = atof(argv[3]);
   cap_len = atof(argv[4]);//added for capturing time
   rows = HEIGHT;
   cols = WIDTH;

   if(argc == 6) dirfilename = (char *) "dummy";
	else dirfilename = NULL;

   std::string pipeline = "libcamerasrc ! video/x-raw, width=" +
                        std::to_string(WIDTH) +
                        ", height=" + std::to_string(HEIGHT) +
                        ", format=(string)BGR ! videoconvert ! appsink";

   VideoCapture cap(pipeline, CAP_GSTREAMER);

   if (!cap.isOpened()) {
      cerr << "Failed to open pipeline\n";
      return -1;
   }

	cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
   cap.set(CAP_PROP_FRAME_HEIGHT,HEIGHT);

   Mat frame, grayframe;
   clock_t begin, mid, end;
   double time_elapsed, time_capture, time_process;
   int frame_id = 1;

   printf("[INFO] Step mode:\n");
   printf("  - Press ENTER to run Canny and save frameXXX.pgm for %.2f million seconds\n", cap_len*100);
   printf("  - Press ESC to quit\n");
   
   for (;;)
   {
      cap >> frame;
      
      if (frame.empty()) {break;}
      
      imshow("[RAW] capturing.... :)", frame);
      
      int key = waitKey(1);
      if (key == 27) break;
      
      if (key == 13 || key == 10) { //ENTER
         
         for(float timer = 0.0; timer < cap_len; timer+=1){
            begin = clock();//starting of the capturing
            cap >> frame;
            mid = clock();//end(mid) of the capturing
            if (frame.empty()) {
               cerr << "Empty frame\n";
               break;
            }

            imshow("[RAW] capturing.... :)", frame);
            
            cvtColor(frame, grayframe, COLOR_BGR2GRAY);
            image = grayframe.data;

            canny(image, rows, cols, sigma, tlow, thigh, &edge, NULL);
            Mat edgeMat(rows, cols, CV_8UC1, edge);
            imshow("[EDGE] Captured and processed :)", edgeMat);
            
            //save as frame001.pgm, frame002.pgm...
            sprintf(outfilename, "camera_canny_img/frame%03d.pgm", frame_id++);
            write_pgm_image(outfilename, edge, rows, cols, NULL, 255);
            cout << "Saved " << outfilename << endl;
            
            end = clock();//end of the processing
            time_elapsed = (double) (end - begin) / CLOCKS_PER_SEC;
            time_capture = (double) (mid - begin) / CLOCKS_PER_SEC;
            time_process = (double) (end - mid) / CLOCKS_PER_SEC;
            printf("  Capture time:      %.6f s\n", time_capture);
            printf("  Process time:      %.6f s\n", time_process);
            printf("  Capture + Process: %.6f s\n", time_elapsed);
            
            free(edge);
            edge = NULL;
            
            int k = waitKey(100);//waiting 100 ms and key
            if (k == 27) { //ESC
               break;
            }
         }
      }
   }

   /****************************************************************************
   * Perform the edge detection. All of the work takes place here.
   ****************************************************************************/
   // if(VERBOSE) printf("Starting Canny edge detection.\n");
   // if(dirfilename != NULL){
   //    sprintf(composedfname, "camera_s_%3.2f_l_%3.2f_h_%3.2f.fim",
   //    sigma, tlow, thigh);
   //    dirfilename = composedfname;
   // }
   // canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);

   /****************************************************************************
   * Write out the edge image to a file.
   ****************************************************************************/
   // sprintf(outfilename, "camera_s_%3.2f_l_%3.2f_h_%3.2f.pgm", sigma, tlow, thigh);
   // if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
   // if(write_pgm_image(outfilename, edge, rows, cols, NULL, 255) == 0){
   //    fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
   //    exit(1);
   // }
   // end = clock();
   // time_elapsed = (double) (end - begin) / CLOCKS_PER_SEC;
   // time_capture = (double) (mid - begin) / CLOCKS_PER_SEC;
   // time_process = (double) (end - mid) / CLOCKS_PER_SEC;

	//  imshow("[GRAYSCALE] this is you, smile! :)", grayframe);

   // printf("Elapsed time for capturing+processing one frame: %lf + %lf => %lf seconds\n", time_capture, time_process, time_elapsed);
   // printf("FPS: %01lf\n", NFRAME/time_elapsed);

	//  grayframe.data = edge;
   // printf("[INFO] (On the pop-up window) Press ESC to terminate the program...\n");
	//  for(;;){
	// 	 imshow("[EDGE] this is you, smile! :)", grayframe);
	// 	 if( waitKey(10) == 27 ) break; // stop capturing by pressing ESC
	//  }
    //free resrources    
//		grayframe.release();
//    delete image;
    return 0;
}
