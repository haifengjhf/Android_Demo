package com.jhf.test.activity;

import com.freegeek.android.materialbanner.MaterialBanner;
import com.freegeek.android.materialbanner.simple.SimpleViewHolderCreator;
import com.freegeek.android.materialbanner.view.indicator.CirclePageIndicator;
import com.jhf.test.R;
import com.jhf.test.adapter.BannerHolderCreator;
import com.jhf.test.adapter.BannerRecycleAdapter;
import com.jhf.test.data.BannerData;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;

import java.util.ArrayList;
import java.util.List;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager.widget.ViewPager;

public class BannerActivity extends AppCompatActivity {
    private final static String TAG =  "BannerActivity";
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_banner);

        if(true){
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setData(Uri.parse("native://test.jhf/main"));
            startActivity(intent);
            finish();
            return;
        }
//        if(true){
//            Intent intent = new Intent();
//            intent.setClassName("com.jhf.test.app","com.jhf.test.activity.MainActivity");
//            startActivity(intent);
//            finish();
//            return;
//        }


        List<BannerData> dataList = new ArrayList<>();
        dataList.add(new BannerData(R.drawable.iv_advisory_record));
        dataList.add(new BannerData(R.drawable.iv_coming_soon));
        dataList.add(new BannerData(R.drawable.iv_device_manage));
        dataList.add(new BannerData(R.drawable.iv_registration_service));

        RecyclerView recyclerView = findViewById(R.id.banner_list_view);
        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.setAdapter(new BannerRecycleAdapter(this,dataList));
    }
}
