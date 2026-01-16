#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <errno.h>

#include "opencv2/opencv.hpp"
#include "canny_util.h"

using namespace std;
using namespace cv;

#define WIDTH  640
#define HEIGHT 480

static bool ensure_dir(const std::string& path)
{
    // mkdir returns 0 on success, -1 on error
    if (mkdir(path.c_str(), 0755) == 0) return true;
    if (errno == EEXIST) return true;
    perror("mkdir");
    return false;
}

static std::string frame_name(const std::string& outdir, int frame_id)
{
    //frame001.pgm style
    std::ostringstream oss;
    oss << outdir << "/frame" << std::setfill('0') << std::setw(3) << frame_id << ".pgm";
    return oss.str();
}

// ./camera_canny <sigma> <tlow> <thigh> <mode> <value> <outdir>
int main(int argc, char **argv)
{
    if (argc < 7) {
        cerr << "\nUSAGE:\n  "
             << argv[0]
             << " sigma tlow thigh <mode:s|n> <value> <outdir>\n\n"
             << "Examples:\n  "
             << argv[0] << " 1.0 0.1 0.3 s 5 camera_canny_img\n  "
             << argv[0] << " 1.0 0.1 0.3 n 150 camera_canny_img\n\n";
        return 1;
    }

    const float sigma = atof(argv[1]);
    const float tlow  = atof(argv[2]);
    const float thigh = atof(argv[3]);

    const char mode = argv[4][0]; // 's' or 'n'
    const double value = atof(argv[5]); // seconds or frames
    const std::string outdir = argv[6];

    if (!(mode == 's' || mode == 'n')) {
        cerr << "ERROR: mode must be 's' (seconds) or 'n' (frames)\n";
        return 1;
    }
    if (value <= 0) {
        cerr << "ERROR: value must be > 0\n";
        return 1;
    }
    if (!ensure_dir(outdir)) {
        cerr << "ERROR: could not create/verify output directory: " << outdir << "\n";
        return 1;
    }

    const int rows = HEIGHT;
    const int cols = WIDTH;

    //pi 5 and libcamera via GStreamer
    std::string pipeline =
        "libcamerasrc ! video/x-raw, width=" + std::to_string(WIDTH) +
        ", height=" + std::to_string(HEIGHT) +
        ", format=(string)BGR ! videoconvert ! appsink";

    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr << "Failed to open camera pipeline.\n";
        return 1;
    }

    cap.set(CAP_PROP_FRAME_WIDTH,  WIDTH);
    cap.set(CAP_PROP_FRAME_HEIGHT, HEIGHT);

    Mat frame, grayframe;

    cout << "[INFO] Running Canny capture+process\n"
         << "  sigma=" << sigma << " tlow=" << tlow << " thigh=" << thigh << "\n"
         << "  mode=" << mode << " value=" << value << "\n"
         << "  outdir=" << outdir << "\n"
         << "Press ESC anytime to stop early.\n";

    //average FPS
    clock_t cpu_begin_total = clock();
    auto wall_begin_total = std::chrono::steady_clock::now();

    double total_cpu_capture = 0.0;
    double total_cpu_process = 0.0;

    int frame_id = 1;
    int frames_done = 0;

    //for seconds mode, we stop when wall elapsed >= value
    //for frames mode, we stop when frames_done >= value
    while (true)
       {
           //termination checks (before grabbing next frame)
           if (mode == 'n' && frames_done >= (int)value) break;
           if (mode == 's') {
               auto now = std::chrono::steady_clock::now();
               double wall_elapsed = std::chrono::duration<double>(now - wall_begin_total).count();
               if (wall_elapsed >= value) break;
           }

           //esc to stop
           int key0 = waitKey(1);
           if (key0 == 27) break;

           //start CPU timing
           clock_t cpu_begin_cap = clock();
           cap >> frame;
           clock_t cpu_end_cap = clock();

           if (frame.empty()) {
               cerr << "Empty frame received; stopping.\n";
               break;
           }

           //show raw camera feed
           imshow("[RAW]", frame);

           cvtColor(frame, grayframe, COLOR_BGR2GRAY);
           unsigned char* image = grayframe.data;

           //cpu timing
           clock_t cpu_begin_proc = clock();

           unsigned char* edge = nullptr;
           canny(image, rows, cols, sigma, tlow, thigh, &edge, nullptr);

           clock_t cpu_end_proc = clock();

           //save edge image
           std::string outname = frame_name(outdir, frame_id++);
           if (write_pgm_image((char*)outname.c_str(), edge, rows, cols, NULL, 255) == 0) {
               cerr << "Error writing " << outname << "\n";
               free(edge);
               break;
           }

           //show edge
           Mat edgeMat(rows, cols, CV_8UC1, edge);
           imshow("[EDGE]", edgeMat);

           //accumulate CPU times
           double cpu_cap_s  = (double)(cpu_end_cap  - cpu_begin_cap)  / CLOCKS_PER_SEC;
           double cpu_proc_s = (double)(cpu_end_proc - cpu_begin_proc) / CLOCKS_PER_SEC;
           total_cpu_capture += cpu_cap_s;
           total_cpu_process += cpu_proc_s;

           frames_done++;

           //per-frame
           cout << "Saved " << outname
                << " | CPU capture=" << fixed << setprecision(6) << cpu_cap_s
                << " s, CPU process=" << cpu_proc_s << " s\n";

           free(edge);
           edge = nullptr;

           //esc
           int key = waitKey(1);
           if (key == 27) break;
       }
       
       //add up end times
       clock_t cpu_end_total = clock();
       auto wall_end_total = std::chrono::steady_clock::now();

       double total_cpu = (double)(cpu_end_total - cpu_begin_total) / CLOCKS_PER_SEC;
       double total_wall = std::chrono::duration<double>(wall_end_total - wall_begin_total).count();

       //average FPS
       //FPS = frames / total_time
       double avg_fps_wall = (frames_done > 0 && total_wall > 0.0) ? (frames_done / total_wall) : 0.0;
       double avg_fps_cpu  = (frames_done > 0 && total_cpu  > 0.0) ? (frames_done / total_cpu)  : 0.0;

       cout << "\n========== SUMMARY ==========\n";
       cout << "Frames processed: " << frames_done << "\n";
       cout << "Total WALL time (steady_clock): " << fixed << setprecision(6) << total_wall << " s\n";
       cout << "Total CPU time (clock):         " << total_cpu  << " s\n";
       cout << "Total CPU capture time:         " << total_cpu_capture << " s\n";
       cout << "Total CPU process time:         " << total_cpu_process << " s\n";
       cout << "Avg FPS (WALL):                 " << avg_fps_wall << "\n";
       cout << "Avg FPS (CPU):                  " << avg_fps_cpu << "\n";
       cout << "Output folder:                  " << outdir << "\n";
       cout << "================================\n";

       return 0;
}
