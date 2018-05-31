package com.example.ffmpegdecoder.activity;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.example.ffmpegdecoder.R;
import com.example.ffmpegdecoder.bean.VideoInfo;

import java.lang.ref.WeakReference;

public class MainActivity extends Activity {

	private final String PERMISSION_WRITE_EXTERNAL_STORAGE= "android.permission.WRITE_EXTERNAL_STORAGE";
	private final int PERMISSION_REQUESTCODE = 0;
	private final int GET_CONTENT_REQUESTCODE = 0;
	private final int OUTPUT_URL_YUV_SUBSTRING_BEGIN_INDEX = 6;
	private final String TAG = this.getClass().getSimpleName();
	private Toast mToast;
	private boolean isShow;

	private ImageView img_decode;
	private RelativeLayout btn_decode;
	private ProgressBar progressBar;

	private RadioGroup decode_method;
	private RadioButton ffmpeg;
	private RadioButton mediacodec;
	private boolean isFFmpeg = true;

	private VideoInfo videoInfo;
	private String input_url;
	private TextView text_input;
	private EditText text_output;
	private TextView text_duration;
	private TextView text_bit_rate;
	private TextView text_frame_rate;
	private TextView text_format_name;
	private TextView text_width;
	private TextView text_height;
	private TextView text_codec_name;
	private TextView text_pixel_format;
	private LinearLayout layout_yuv;
	private boolean isSelect = false;
	private Handler progressRateHandler = new progressRateHandler(this);
	private Thread decodeThread;

	long time_start;
	long time_end;
	int frame_count;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

		btn_decode = (RelativeLayout) this.findViewById(R.id.ly_decode);
		img_decode = (ImageView) this.findViewById(R.id.btn_decode);
		btn_decode.setClickable(false);
		toggleBtnDecode(false);

		progressBar = (ProgressBar) this.findViewById(R.id.progressBar);
		progressBar.setVisibility(View.INVISIBLE);
		text_input= (TextView) this.findViewById(R.id.input_url);
		layout_yuv = (LinearLayout) this.findViewById(R.id.ly_yuv);
		layout_yuv.setAlpha(0.5f);
		text_output= (EditText) this.findViewById(R.id.output_url);
		text_output.setEnabled(false);
		text_duration = (TextView) this.findViewById(R.id.duration);
		text_bit_rate = (TextView) this.findViewById(R.id.bit_rate);
		text_frame_rate = (TextView) this.findViewById(R.id.frame_rate);
		text_format_name= (TextView) this.findViewById(R.id.format_name);
		text_width = (TextView) this.findViewById(R.id.width);
		text_height = (TextView) this.findViewById(R.id.height);
		text_codec_name = (TextView) this.findViewById(R.id.codec_name);
		text_pixel_format = (TextView) this.findViewById(R.id.pixel_format);

		text_input.setOnClickListener(new OnClickListener() {
			public void onClick(View arg0) {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
					if(checkSelfPermission(PERMISSION_WRITE_EXTERNAL_STORAGE)!= PackageManager.PERMISSION_GRANTED){
                        requestPermissions(new String[]{PERMISSION_WRITE_EXTERNAL_STORAGE},PERMISSION_REQUESTCODE);
                    }
                    else{
                        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
                        intent.setType("*/*");
                        intent.addCategory(Intent.CATEGORY_OPENABLE);
                        startActivityForResult(Intent.createChooser(intent,"请选择文件管理器"),GET_CONTENT_REQUESTCODE);
                    }
				}
				else{
					Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
					intent.setType("*/*");
					intent.addCategory(Intent.CATEGORY_OPENABLE);
					startActivityForResult(Intent.createChooser(intent,"请选择文件管理器"),GET_CONTENT_REQUESTCODE);
				}
			}
		});

		btn_decode.setOnClickListener(new OnClickListener() {
			public void onClick(View arg0){
//				String folderurl=Environment.getExternalStorageDirectory().getPath();
				if(isSelect) {
					final String urltext_input = text_input.getText().toString();
					final String urltext_output = text_output.getText().toString().substring(OUTPUT_URL_YUV_SUBSTRING_BEGIN_INDEX);

					Log.i("inputurl", urltext_input);
					Log.i("outputurl", urltext_output);

					if(!"h264".equals(videoInfo.format_name))
						progressBar.setVisibility(View.VISIBLE);

					decodeThread = new Thread() {
						@Override
						public void run() {
							time_start = System.currentTimeMillis();
							if(isFFmpeg) {
								decode(urltext_input, urltext_output, 0);
//								decode("rtsp://192.168.1.188/11", "/storage/emulated/0/testVideo/111_640x480.yuv");
							}
							else {
								decode(urltext_input, urltext_output, 1);
							}
						}
					};
					decodeThread.start();
					btn_decode.setClickable(false);
					toggleBtnDecode(false);
				}
				else{
					showToast("请先选择视频路径",Toast.LENGTH_SHORT);
				}
			}
		});

		decode_method = (RadioGroup) this.findViewById(R.id.decode_method);
		ffmpeg = (RadioButton) this.findViewById(R.id.ffmpeg);
		mediacodec = (RadioButton) this.findViewById(R.id.mediacodec);

		decode_method.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(RadioGroup group, int checkedId) {
				if(checkedId == ffmpeg.getId()){
					isFFmpeg = true;
				}
				else if(checkedId == mediacodec.getId()){
					isFFmpeg = false;
				}

				if(input_url != null) {
					boolean isSetPixelWandH = false;
					int _Index = input_url.lastIndexOf("_");
					int xIndex = input_url.lastIndexOf("x");
					int pointIndex = input_url.lastIndexOf(".");
					Log.i(TAG, "onCheckedChanged: _Index="+_Index+" xIndex="+xIndex+" pointIndex="+pointIndex);
					if (_Index < xIndex && xIndex < pointIndex) {
						try {
							Integer.parseInt(input_url.substring(_Index+1, xIndex));
							Integer.parseInt(input_url.substring(xIndex+1, pointIndex));
							isSetPixelWandH = true;
						} catch (Exception e) {
							e.printStackTrace();
							isSetPixelWandH = false;
						}
					}

					int pixel_type = -1;
					if (isFFmpeg) {
						pixel_type = 0;
					} else {
						pixel_type = 5;
					}

					if (isSetPixelWandH) {
						int _index = input_url.lastIndexOf("_");
						text_output.setText("yuv输出：" + input_url.substring(0, _index) + "&" + pixel_type
								+ "_" + videoInfo.width + "x" + videoInfo.height + ".yuv");
					} else {
						text_output.setText("yuv输出：" + input_url.substring(0, input_url.lastIndexOf("."))
								+ "&" + pixel_type + "_" + videoInfo.width + "x" + videoInfo.height + ".yuv");
					}
					text_output.setSelection(text_output.getText().toString().length());
				}
			}
		});
    }

	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
		switch (requestCode) {
			case PERMISSION_REQUESTCODE:
				if ("android.permission.WRITE_EXTERNAL_STORAGE".equals(permissions[0])
						&& grantResults[0] == PackageManager.PERMISSION_GRANTED) {
					Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
					intent.setType("video/*");
					intent.addCategory(Intent.CATEGORY_OPENABLE);
					startActivityForResult(Intent.createChooser(intent,"请选择文件管理器"),0);
				}
		}
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if(resultCode == Activity.RESULT_OK){
			switch (requestCode){
				case GET_CONTENT_REQUESTCODE:
					Uri uri = data.getData();
					input_url = uri.getPath();
					if("/root".equals(input_url.substring(0,5))){
						input_url = input_url.substring(5);
					}

					videoInfo = new VideoInfo();
					int stutas = decodeInfo(videoInfo,input_url);
//					int stutas = decodeInfo(videoInfo,"rtsp://192.168.1.188/11");
					Log.i("videoInfo: ", videoInfo.toString());
					if(stutas != 0) {
						switch (stutas) {
							case -1:
								showToast( "无法打开视频文件，请检查读写权限或文件是否损坏！", Toast.LENGTH_SHORT);
								break;
							case -2:
								showToast( "无法从该文件获取流信息，请确认是否打开视频文件！", Toast.LENGTH_SHORT);
								break;
							case -3:
								showToast( "无法从该文件获取视频流信息，该封装格式无视频流！", Toast.LENGTH_SHORT);
								break;
							case -4:
								showToast( "无法获取正确的解码器，该视频文件被损坏！", Toast.LENGTH_SHORT);
								break;
							case -5:
								showToast( "无法打开解码器，该视频文件被损坏！", Toast.LENGTH_SHORT);
								break;
						}
					}
					else {
						isSelect = true;
						layout_yuv.setAlpha(1);
						text_output.setEnabled(true);
						text_input.setText(input_url);

						boolean isSetPixelWandH = false;
						int _Index = input_url.lastIndexOf("_");
						int xIndex = input_url.lastIndexOf("x");
						int pointIndex = input_url.lastIndexOf(".");
						Log.i(TAG, "onActivityResult: _Index="+_Index+" xIndex="+xIndex+" pointIndex="+pointIndex);
						if (_Index < xIndex && xIndex < pointIndex) {
							try {
								Integer.parseInt(input_url.substring(_Index+1, xIndex));
								Integer.parseInt(input_url.substring(xIndex+1, pointIndex));
								isSetPixelWandH = true;
							} catch (Exception e) {
								e.printStackTrace();
								isSetPixelWandH = false;
							}
						}

						int pixel_type = -1;
						if(isFFmpeg){
							pixel_type = 0;
						}else{
							pixel_type = 5;
						}

						if(isSetPixelWandH){
							int _index = input_url.lastIndexOf("_");
							text_output.setText("yuv输出：" + input_url.substring(0, _index) + "&" + pixel_type
									+ "_"+videoInfo.width+"x"+videoInfo.height+".yuv");
						}
						else {
							text_output.setText("yuv输出：" + input_url.substring(0, input_url.lastIndexOf("."))
									+ "&" + pixel_type + "_" + videoInfo.width + "x" + videoInfo.height + ".yuv");
						}
						text_output.setSelection(text_output.getText().toString().length());

						btn_decode.setClickable(true);
						toggleBtnDecode(true);

						if (!"h264".equals(videoInfo.format_name)) {
							text_duration.setText(text_duration.getHint().toString() + " " + String.format("%.2f",videoInfo.duration) + " s");
							text_bit_rate.setText(text_bit_rate.getHint().toString() + " " + ((double) videoInfo.bit_rate / 1000) + " kbps");
							text_frame_rate.setText(text_frame_rate.getHint().toString() + " " + String.format("%.2f", videoInfo.frame_rate) + " fps");
							text_format_name.setText(text_format_name.getHint().toString() + " " + videoInfo.format_name);
							text_width.setText(text_width.getHint().toString() + " " + videoInfo.width + " px");
							text_height.setText(text_height.getHint().toString() + " " + videoInfo.height + " px");
							text_codec_name.setText(text_codec_name.getHint().toString() + " " + videoInfo.codec_name);
							text_pixel_format.setText(text_pixel_format.getHint().toString() + " " + videoInfo.pixel_format);

							progressBar.setMax((int) (videoInfo.duration * videoInfo.frame_rate));
							progressBar.setProgress(0);
						} else {
							text_duration.setText(text_duration.getHint().toString() + " H264裸流不包含时长信息");
							text_bit_rate.setText(text_bit_rate.getHint().toString() + " H264裸流不包含比特率信息");
							text_frame_rate.setText(text_frame_rate.getHint().toString() + " H264裸流不包含帧率信息");
							text_format_name.setText(text_format_name.getHint().toString() + " " + videoInfo.format_name);
							text_width.setText(text_width.getHint().toString() + " " + videoInfo.width + " px");
							text_height.setText(text_height.getHint().toString() + " " + videoInfo.height + " px");
							text_codec_name.setText(text_codec_name.getHint().toString() + " " + videoInfo.codec_name);
							text_pixel_format.setText(text_pixel_format.getHint().toString() + " " + videoInfo.pixel_format.trim());

							progressBar.setVisibility(View.INVISIBLE);
							progressBar.setProgress(0);
						}

						if(!"h264".equals(videoInfo.codec_name)){
							mediacodec.setEnabled(false);
							decode_method.check(R.id.ffmpeg);
							text_codec_name.setText(text_codec_name.getText()+"\n(该格式不支持硬解码)");
						}
						else{
							mediacodec.setEnabled(true);
						}
					}
				break;
			}
		}
	}

	static class progressRateHandler extends Handler {
		WeakReference<MainActivity> mActivityReference;
		progressRateHandler(MainActivity activity) {
			mActivityReference= new WeakReference(activity);
		}
		@Override
		public void handleMessage(Message msg) {
			final MainActivity activity = mActivityReference.get();
			if (activity != null) {
				if(msg.what == -1) {
					activity.progressBar.setProgress(activity.progressBar.getMax());
					activity.showToast("视频解码完成~~", Toast.LENGTH_SHORT);
					activity.btn_decode.setClickable(true);
					activity.toggleBtnDecode(true);
				}
				else if(msg.what == -2) {
					activity.progressBar.setProgress(0);
					activity.showToast("视频解码取消！！", Toast.LENGTH_SHORT);
					activity.btn_decode.setClickable(true);
					activity.toggleBtnDecode(true);
				}
				else {
					if(activity.isShow) {
						activity.showToast("正在解码第 " + msg.what + " 帧", Toast.LENGTH_SHORT);
					}
					activity.progressBar.setProgress(msg.what);
				}
			}
		}
	}


	//JNI
	public void setProgressRate(int progress){
		frame_count = progress;
		progressRateHandler.sendEmptyMessage(progress);
	}

	public void setProgressRateFull(){
		time_end = System.currentTimeMillis();
		frame_count++;
		Log.i("frame_count", ""+frame_count);
		Log.i("decode_time", ""+((double)(time_end - time_start))/1000);
		progressRateHandler.sendEmptyMessage(-1);
	}
	public void setProgressRateEmpty(){
		progressRateHandler.sendEmptyMessage(-2);
	}

    public native int decode(String inputurl, String outputurl, int codec_type);
	public native int decodeInfo(VideoInfo videoInfo,String inputurl);
	public native void decodeCancel();

    static{
//    	System.loadLibrary("avutil-54");
//    	System.loadLibrary("swresample-1");
//    	System.loadLibrary("avcodec-56");
//    	System.loadLibrary("avformat-56");
//    	System.loadLibrary("swscale-3");
//    	System.loadLibrary("postproc-53");
//    	System.loadLibrary("avfilter-5");
//    	System.loadLibrary("avdevice-56");
    	System.loadLibrary("ffmpegdecoder");
		System.loadLibrary("ffmpeg");
    }

	void toggleBtnDecode(boolean isClickable){
		if(isClickable)
			img_decode.setImageResource(R.drawable.ic_export_send);
		else
			img_decode.setImageResource(R.drawable.ic_export_send_gray);
	}
	@Override
	protected void onResume() {
		super.onResume();
		isShow = true;
	}

	@Override
	protected void onPause() {
		super.onPause();
		isShow = false;
		cancelToast();
	}

	/**
	 * 显示Toast，解决重复弹出问题
	 */
	public void showToast(String text , int time) {
		if(mToast == null) {
			mToast = Toast.makeText(this, text, time);
		} else {
			mToast.setText(text);
			mToast.setDuration(Toast.LENGTH_SHORT);
		}
		mToast.show();
	}

	/**
	 * 隐藏Toast
	 */
	public void cancelToast() {
		if (mToast != null) {
			mToast.cancel();
			mToast = null;
		}
	}


	public void onBackPressed() {
		if(decodeThread != null && decodeThread.isAlive()){
			decodeCancel();
			cancelToast();
		}
		else{
			cancelToast();
			super.onBackPressed();
		}
	}
}