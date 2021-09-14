package com.jhf.test.view;



import com.jhf.test.R;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;

import androidx.constraintlayout.widget.ConstraintLayout;

public class ImgConstraintLayout extends ConstraintLayout implements View.OnFocusChangeListener {
    private static final String TAG = "ImgConstraintLayout";
    private ValueAnimator valueAnimator;
    private boolean isMove = true;

    public ImgConstraintLayout(Context context) {
        this(context, null);
    }

    public ImgConstraintLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ImgConstraintLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context);
    }

    private void init(Context context) {
        LayoutInflater.from(context).inflate(R.layout.layout_img_constra, this);
        setFocusable(true);
        setOnFocusChangeListener(this);
    }

    public void setMove(boolean isMove) {
        this.isMove = isMove;
    }

    @Override
    public void onFocusChange(View v, boolean hasFocus) {
        Log.e(TAG, "onFocusChange: 0000  " + hasFocus);
        if (v != null) {
            if (hasFocus) {
                Log.e(TAG, "onFocusChange: 1111  " + hasFocus);
                if (isMove) {
                    move(v.findViewById(R.id.light));
                }
            } else {
                remove();
            }
        }
    }

    private void move(final View view) {
        view.bringToFront();
        final int width = getWidth();
        valueAnimator = ValueAnimator.ofFloat(((Integer) (-width)).floatValue(), ((Integer) (width)).floatValue());
        valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator animation) {
                float aFloat = (float) animation.getAnimatedValue();
                view.setTranslationX(aFloat);
                float alpha = aFloat / width;
                float a1 = (alpha > 0 ? (1 - alpha) : (1 + alpha));
                float a2 = (float) (a1 / 2 + 0.3);
                view.setAlpha(a2);
            }
        });
        valueAnimator.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                view.setVisibility(VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                view.setVisibility(GONE);

            }

            @Override
            public void onAnimationCancel(Animator animation) {

            }

            @Override
            public void onAnimationRepeat(Animator animation) {

            }
        });
        valueAnimator.setInterpolator(new AccelerateDecelerateInterpolator());
        int d = width / 355 - 1;
        float ff = 1000 * (d * 0.25f + 1);
        valueAnimator.setDuration(((Float) ff).longValue());
        valueAnimator.setStartDelay(300);
        valueAnimator.start();
    }

    private void remove() {
        if (valueAnimator != null) {
            valueAnimator.cancel();
        }
    }
}
