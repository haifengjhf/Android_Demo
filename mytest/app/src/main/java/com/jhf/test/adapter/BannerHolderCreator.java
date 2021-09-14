package com.jhf.test.adapter;

import com.freegeek.android.materialbanner.holder.ViewHolderCreator;

public class BannerHolderCreator implements ViewHolderCreator<BannerHolder> {
    @Override
    public BannerHolder createHolder() {
        return new BannerHolder();
    }
}
