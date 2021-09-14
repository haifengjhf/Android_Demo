package com.jhf.test.adapter;

import com.freegeek.android.materialbanner.holder.Holder;
import com.jhf.test.R;
import com.jhf.test.data.BannerData;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;

public class BannerHolder implements Holder<BannerData> {
    private ImageView imageView;

    @Override
    public View createView(Context context) {
        View itemView =  LayoutInflater.from(context).inflate(R.layout.item_banner,null,false);
        imageView = itemView.findViewById(R.id.banner_image);
        return itemView;
    }

    @Override
    public void updateUI(Context context, int position, BannerData data) {
        if(data != null && data.imgResId != 0 ){
            imageView.setImageResource(data.imgResId);
        }
    }
}
