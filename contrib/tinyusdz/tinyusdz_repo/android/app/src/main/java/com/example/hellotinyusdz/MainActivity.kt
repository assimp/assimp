/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package com.example.hellotinyusdz

import android.content.res.AssetManager
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.view.GestureDetector
import android.widget.ImageView
import android.widget.Toast
import android.graphics.Bitmap
import android.util.Log

import androidx.core.view.MotionEventCompat

import android.graphics.Color
import android.graphics.BitmapFactory
import kotlinx.android.synthetic.main.activity_main.sample_text

class MainActivity : AppCompatActivity(), GestureDetector.OnGestureListener {

    val render_width = 512
    val render_height = 512
    var lastTouchTime: Long = -1
    var motionX = -1.0f
    var motionY = -1.0f

    fun updateRender() {
        renderImage(render_width, render_height);

        var conf = Bitmap.Config.ARGB_8888
        var b = Bitmap.createBitmap(render_width, render_height, conf)

        var pixels = IntArray(render_width * render_height)

        grabImage(pixels, render_width, render_height)

        b.setPixels(pixels, 0, render_width, 0, 0, render_width, render_height)

        var img = findViewById<ImageView>(R.id.imageView)

        img.setImageBitmap(b)
    }


    override fun onLongPress(event: MotionEvent) {
        Log.d("tinyusdz", "onLongPress: $event")
    }

    override fun onScroll(
        event1: MotionEvent,
        event2: MotionEvent,
        distanceX: Float,
        distanceY: Float
    ): Boolean {
        Log.d("tinyusdz", "onScroll: $event1 $event2")
        return true
    }

    override fun onShowPress(event: MotionEvent) {

        Log.d("tinyusdz", "onShowPress: $event")
    }

    override fun onSingleTapUp(event: MotionEvent): Boolean {
        Log.d("tinyusdz", "onSingleTapUp: $event")
        return true
    }

    override fun onDown(event: MotionEvent): Boolean {
        Log.d("tinyusdz", "onDown: $event")
        return true
    }

    override fun onFling(
        event1: MotionEvent,
        event2: MotionEvent,
        velocityX: Float,
        velocityY: Float
    ): Boolean {
        Log.d("tinyusdz", "onFling: $event1 $event2")
        return true
    }

    override fun onTouchEvent(event: MotionEvent) : Boolean {
        val action: Int = MotionEventCompat.getActionMasked(event)

        return when (action) {
            MotionEvent.ACTION_DOWN -> {
                Log.d("tinyusdz", "Action was DOWN")
                motionX = event.getX()
                motionY = event.getY()

                true
            }
            MotionEvent.ACTION_MOVE -> {
                var x = event.getX()
                var y = event.getY()

                if ((lastTouchTime < 0) || (event.eventTime < lastTouchTime)) {
                    motionX = x
                    motionY = y
                }

                var eventDuration = event.eventTime - event.downTime;
                lastTouchTime = eventDuration


                var dx = x - motionX
                var dy = y - motionY

                Log.d("tinyusdz", "Action was MOVE. duration " + eventDuration + ", dx " + dx + ", dy = " + dy)

                touchMove(dx, dy)

                updateRender()

                motionX = x
                motionY = y

                true
            }
            MotionEvent.ACTION_UP -> {
                Log.d("tinyusdz", "Action was UP")

                motionX = event.getX()
                motionY = event.getY()

                true
            }
            MotionEvent.ACTION_CANCEL -> {
                Log.d("tinyusdz", "Action was CANCEL")
                true
            }
            MotionEvent.ACTION_OUTSIDE -> {
                Log.d("tinyusdz", "Movement occurred outside bounds of current screen element")
                true
            }
            else -> {
                Log.d("tinyusdz", "Another action")
                super.onTouchEvent(event)
            }
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val view = findViewById<View>(R.id.container)


        // Set up a touch listener which calls the native sound engine
        view.setOnTouchListener {_, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                //playSound(true)
            } else if (event.action == MotionEvent.ACTION_UP) {
                //playSound(false)
            } else {
                return@setOnTouchListener false
            }
            true
        }
    }

    override fun onResume() {
        super.onResume()

        var n = initScene(getAssets(), "suzanne.usdc");

        if (n <= 0) {
            val errorString : String = "Failed to load USD file"
            Toast.makeText(applicationContext, errorString,Toast.LENGTH_LONG).show()
            sample_text.text = errorString
        } else {
            val s : String = "Loaded USD. # of geom_meshes " + n
            Toast.makeText(applicationContext, s,Toast.LENGTH_LONG).show()
            sample_text.text = s

            updateRender()
        }
    }

    override fun onPause() {
        super.onPause()
    }


    private external fun initScene(mgr: AssetManager, filename: String) : Int
    private external fun renderImage(width: Int, height: Int) : Int
    private external fun grabImage(img: IntArray, width: Int, height: Int) : Int
    private external fun touchMove(dx: Float, dy: Float) : Int

    companion object {
        // Used to load native code calling oboe on app startup.
        init {
            System.loadLibrary("hello-tinyusdz")
        }
    }
}
