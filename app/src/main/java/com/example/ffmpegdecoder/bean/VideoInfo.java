package com.example.ffmpegdecoder.bean;

/**
 * Created by h26376 on 2017/8/24.
 */

public class VideoInfo {
    public double duration;
    public int bit_rate;
    public double frame_rate;
    public String format_name;
    public int width;
    public int height;
    public String codec_name;
    public String pixel_format;

    @Override
    public String toString() {
        return "VideoInfo{" +
                "duration=" + duration +
                ", bit_rate=" + bit_rate +
                ", frame_rate=" + frame_rate +
                ", format_name='" + format_name + '\'' +
                ", width=" + width +
                ", height=" + height +
                ", codec_name='" + codec_name + '\'' +
                '}';
    }
}
