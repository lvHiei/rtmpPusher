# rtmpPusher
push audio and video data to rtmp server with librtmp.
Now it support compile for Android and Ubuntu, 
you can set the ANDROID_PROJECT value to 1 for compile Android so in CMakeLists.txt.



Usage:

                                +-----------+
                                |           |               
                +---------------|   Init    |---------------+
                |               |           |               |
                |               +-----------+               |
                |                                           |
               \ /                                         \ /
    +-----------------------+                   +-----------------------+           
    |                       |                   |                       |
    |    sendSpsAndPps      |                   | sendAacSequenceHeader |
    |                       |                   |                       |
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
        |               |             |             |                |
        | sendVideoData |             |             | sendAudioData  |
        |               |             |             |                |
        +---------------+             |             +----------------+        
                                      |
                                      |        
                                      |   
                                      |
                                      |
                                      |
                                      |
                                      |
                                     \ /   
                                +-----------+
                                |           |
                                |   stop    |
                                |           |
                                +-----------+        