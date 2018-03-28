# rtmpPusher
push audio and video data to rtmp server with librtmp.
Now it support compile for Android and Ubuntu, 
you can set the ANDROID_PROJECT value to 1 for compile Android so in CMakeLists.txt.

In RtmpPusher, there is comment in every bit of tag header according to the ISO 14496
It is a very helpful for learning the flv muxer that contains h264 and aac data.


Usage:

                                +-----------+
                +---------------|   Init    |---------------+
                |               +-----------+               |
                |                                           |
               \ /                                         \ /
    +-----------------------+                   +-----------------------+           
    |    sendSpsAndPps      |                   | sendAacSequenceHeader |
    +-----------------------+                   +-----------------------+
                |                                           |
                |                                           |
                |                                           |
                +-------------------------------------------+
                                      |                                       
                                      |
                                      |    
                                      |
                ----------------------+---------------------+
                |                     |                     |  
               \ /                    |                    \ / 
        +---------------+             |             +----------------+           
        | sendVideoData |             |             | sendAudioData  |
        +---------------+             |             +----------------+        
                                      |
                                      |        
                                      |
                                     \ /   
                                +-----------+
                                |   stop    |
                                +-----------+        