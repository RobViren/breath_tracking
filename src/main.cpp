#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <chrono>

//globals for thread lazyness
std::atomic<int> BPM(20);
std::atomic<bool> is_running(true);

using namespace cv;

int main(int argc, char** argv)
{
    //Capture stream frm webcam
    VideoCapture cap;

    if(!cap.open(0))
        return 0;

    //Opencv
    Mat current_frame;
    Mat compare_frame;
    Mat subtracted_frame;

    //Count function
    int loop_control;
    int thresh_value = 50;
    int max_count;
    int max_max_count = 40000;
    int min_max_count = 4000;
    int running_ave = 1500;
    float alpha = .02;

    //Breathing function
    float breath_alpha = .1;
    float seconds_per_breath = 3.f;
    //Conditions for correct measurements
    bool is_above = false;
    bool is_below = false;
    bool is_there = false;
    bool is_stable = false;

    //Timer for breath calcs
    std::chrono::high_resolution_clock::time_point start;

    //presence monitoring varible
    int frames_since_last_move = 0;
    int frames_of_low_movement = 0;

    while(is_running)
    {
        //Capture an image
        cap >> current_frame;
        cvtColor(current_frame, current_frame, CV_BGR2GRAY);

        //Fill current frame so it doesnt crash
        if(compare_frame.empty())
            current_frame.copyTo(compare_frame);

        //Brek if something went wrong
        if( current_frame.empty())
           break;

        //Assign the subtracted frame
        subtracted_frame = current_frame - compare_frame;

        //Make binary
        int total_count = 0;
        for(int i = 0; i < subtracted_frame.rows; ++i){
            for(int j = 0; j < subtracted_frame.cols; ++j){
                if(subtracted_frame.at<uchar>(i,j) > thresh_value){
                    subtracted_frame.at<uchar>(i,j) = 255;
                    total_count++;
                }
                else{
                    subtracted_frame.at<uchar>(i,j) = 0;
                }
            }
        }

        //update accumulator
        running_ave = (alpha * total_count) + (1.0 - alpha) * running_ave;
        max_count = 3 * running_ave;

        //Check max count
        if(max_count > max_max_count)
            max_count = max_max_count;
        else if(max_count < min_max_count)
            max_count = min_max_count;

        //Iterate presence var
        frames_since_last_move++;

        //Check for large movement
        if(total_count > max_count){
            current_frame.copyTo(compare_frame);
            frames_since_last_move = 0;
        }

        //Breathing logic-------------------------------------
        //held still for 5 sec. Should be enough for running ave
        if(frames_since_last_move > 120 && !is_stable){
            start = std::chrono::high_resolution_clock::now();
            is_stable = true;
        }
        else if(frames_since_last_move <= 120){
            is_stable = false;
        }

        //Check that they are there
        if(abs(total_count - running_ave) < 500)
            frames_of_low_movement++;
        else
            frames_of_low_movement = 0;

        if(frames_of_low_movement > 300)
            is_there = false;
        else
            is_there = true;

        //Logic if they are there and still enough to check
        if(is_stable && is_there){
            if(total_count > running_ave + 700){
                is_above = true;
            }
            if(total_count < running_ave - 700){
                is_below = true;
            }

            if(is_above && is_below){
                is_above = false;
                is_below = false;
                std::chrono::duration<float> time_since_last_breath = std::chrono::duration_cast<std::chrono::duration<float>>(start - std::chrono::high_resolution_clock::now());
                start = std::chrono::high_resolution_clock::now();

                //Ensure reasonable breathing rate measurement 5 ~ 40 ish
                if(time_since_last_breath.count() * -1 > 1.5f && time_since_last_breath.count() * -1 < 12.f){
                    seconds_per_breath = (breath_alpha * time_since_last_breath.count() * -1) + (1.0 - breath_alpha) * seconds_per_breath;
                }

                std::cout << "Breath taken " << time_since_last_breath.count() * -1 << " average = " << seconds_per_breath  << "\n";

            }
        }
        else{
            is_above = false;
            is_below = false;
        }
        //Assign BPM
        BPM = 60/seconds_per_breath;

        //Add running ave to image
        putText(subtracted_frame, std::to_string(running_ave), Point(50,50), FONT_HERSHEY_PLAIN, 5, Scalar(200,200,200));
        putText(subtracted_frame, std::to_string(total_count), Point(50,150), FONT_HERSHEY_PLAIN, 5, Scalar(200,200,200));
        putText(subtracted_frame, std::to_string(seconds_per_breath), Point(50,250), FONT_HERSHEY_PLAIN, 5, Scalar(200,200,200));
        putText(subtracted_frame, std::to_string(BPM), Point(50,350), FONT_HERSHEY_PLAIN, 5, Scalar(200,200,200));

        //Display image
        imshow("Breath Tracking", subtracted_frame);

        //Loop logic
        loop_control = waitKey(1);
        if(loop_control == 27)
            is_running = false;
        else if(loop_control == 32)
            current_frame.copyTo(compare_frame);

    }

    cap.release();
    return 0;

}
