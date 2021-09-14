package com.jhf.test.adapter;

import com.freegeek.android.materialbanner.MaterialBanner;
import com.freegeek.android.materialbanner.view.indicator.CirclePageIndicator;
import com.jhf.test.R;
import com.jhf.test.data.BannerData;

import android.content.Context;
import android.media.MediaPlayer;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.List;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;

public class BannerRecycleAdapter extends RecyclerView.Adapter {
    private final static String TAG = "BannerRecycleAdapter";
    private List<BannerData> mDataList;
    private Context mContext;

    public BannerRecycleAdapter(Context context,List<BannerData> dataList){
        mDataList = dataList;
        mContext = context;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        return new RecycleViewHolder(mContext,LayoutInflater.from(mContext).inflate(R.layout.item_recycle,parent,false),mDataList);
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {

    }

    @Override
    public int getItemCount() {
        return mDataList.size();
    }

    public static class RecycleViewHolder extends RecyclerView.ViewHolder{
        private List<BannerData> mDataList;
        private Context mContext;
        public RecycleViewHolder(Context context,@NonNull View itemView,List<BannerData> dataList) {
            super(itemView);
            mDataList = dataList;
            mContext = context;

            MaterialBanner materialBanner = (MaterialBanner) itemView.findViewById(R.id.material_banner);
            materialBanner.setPages(new BannerHolderCreator(), mDataList)
                    .setIndicator(new CirclePageIndicator(mContext));
            //set circle indicator
            materialBanner.setIndicator(new CirclePageIndicator(mContext));

            materialBanner.setOnPageChangeListener(new ViewPager.OnPageChangeListener() {
                @Override
                public void onPageScrolled(int position, float positionOffset, int positionOffsetPixels) {
//                Log.d(TAG,"onPageScrolled :" + position);
                }

                @Override
                public void onPageSelected(int position) {
                    Log.d(TAG,"onPageSelected :" + position);
                }

                @Override
                public void onPageScrollStateChanged(int state) {
                    Log.d(TAG,"onPageScrollStateChanged :" + state);
                }
            });

            materialBanner.setOnFocusChangeListener(new View.OnFocusChangeListener() {
                @Override
                public void onFocusChange(View v, boolean hasFocus) {
                    Log.d(TAG,"onFocusChange v:" + v + " hasFocus:" + hasFocus);
                }
            });

            materialBanner.startTurning(3000);
        }
    }

}
