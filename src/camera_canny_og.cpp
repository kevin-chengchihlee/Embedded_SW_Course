/**
*/
#include <ctime>
#include <iostream>
#include <unistd.h>
#include "opencv2/opencv.hpp"
#include "canny_util.h"

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

   /****************************************************************************
   * Get the command line arguments.
   ****************************************************************************/
   if(argc < 4){
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
   rows = HEIGHT;
   cols = WIDTH;

   if(argc == 5) dirfilename = (char *) "dummy";
	 else dirfilename = NULL;

   VideoCapture cap;
   // open the default camera (/dev/video0)
   // Check VideoCapture documentation for more details
   if(!cap.open(0)){
      cout<<"Failed to open /dev/video0"<<endl;
        return 0;
   }
   cap.set(CAP_PROP_FRAME_WIDTH, WIDTH);
   cap.set(CAP_PROP_FRAME_HEIGHT,HEIGHT);

   Mat frame, grayframe;

   printf("[INFO] (On the pop-up window) Press ESC to start Canny edge detection...\n");
      for(;;)
   {
      cap >> frame;
      if( frame.empty() ) break; // end of video stream
      imshow("[RAW] this is you, smile! :)", frame);
      if( waitKey(10) == 27 ) break; // stop capturing by pressing ESC
   }

   clock_t begin, mid, end;
   double time_elapsed, time_capture, time_process;

   begin = clock();
   //capture
   cap >> frame;
   mid = clock();
   cvtColor(frame, grayframe, COLOR_BGR2GRAY);
   image = grayframe.data;

   /****************************************************************************
   * Perform the edge detection. All of the work takes place here.
   ****************************************************************************/
   if(VERBOSE) printf("Starting Canny edge detection.\n");
   if(dirfilename != NULL){
      sprintf(composedfname, "camera_s_%3.2f_l_%3.2f_h_%3.2f.fim",
      sigma, tlow, thigh);
      dirfilename = composedfname;
   }
   canny(image, rows, cols, sigma, tlow, thigh, &edge, dirfilename);

   /****************************************************************************
   * Write out the edge image to a file.
   ****************************************************************************/
   sprintf(outfilename, "camera_s_%3.2f_l_%3.2f_h_%3.2f.pgm", sigma, tlow, thigh);
   if(VERBOSE) printf("Writing the edge iname in the file %s.\n", outfilename);
   if(write_pgm_image(outfilename, edge, rows, cols, NULL, 255) == 0){
      fprintf(stderr, "Error writing the edge image, %s.\n", outfilename);
      exit(1);
   }
   end = clock();
   time_elapsed = (double) (end - begin) / CLOCKS_PER_SEC;
   time_capture = (double) (mid - begin) / CLOCKS_PER_SEC;
   time_process = (double) (end - mid) / CLOCKS_PER_SEC;

   imshow("[GRAYSCALE] this is you, smile! :)", grayframe);

   printf("Elapsed time for capturing+processing one frame: %lf + %lf => %lf seconds\n", time_capture, time_process, time_elapsed);
   printf("FPS: %01lf\n", NFRAME/time_elapsed);

   grayframe.data = edge;
   printf("[INFO] (On the pop-up window) Press ESC to terminate the program...\n");
   for(;;){
      imshow("[EDGE] this is you, smile! :)", grayframe);
      if( waitKey(10) == 27 ) break; // stop capturing by pressing ESC
   }

    //free resrources    
    //grayframe.release();
    //delete image;
    return 0;
}
