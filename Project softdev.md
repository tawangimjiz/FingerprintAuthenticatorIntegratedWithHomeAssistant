---
title: Project softdev

---

# **Two Factor Authenticator**
## Team members
1. ณฐพงศ์ รอดปรีชา &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;6501012630069
2. วศิน สิงหวงค์    &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 6501012630166
3. ณดล หุณชนะเสวีย์ &nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 6501012630077
4. ตะวัน ลีลาเกียรติวณิช &nbsp;&nbsp;&nbsp;6501012630115
5. ชินบัญชร ยืนมั่น &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 6501012630051

## Expected Task
1. Web , app for OTP system.
2. Client can choose what to do first.
     * OTP system using MQTT broker (subscribe/public).
     * Fingerprint into database check.
3. Verified user's data stored in database (name, date, time).
4. Display client's information through display (optional).
5. Notification to admin on unrecognized attempt. (on esp-home)
## Diagram/Drawings
![456696734_1567188290673966_1063008407720860543_n](https://hackmd.io/_uploads/SksNBTWaC.png)

## Role and responsibility (unassigned)
1. Team leader
2. Dashboard developer
3. Mqtt developer
4. Esp home configurator (raspberry pi)
5. Database manager & reporting!


## Reference
![reference](https://hackmd.io/_uploads/Hk3tr6b6R.png)


## Report on 22 September 2567

Today We tried to connect the finger sensor with Wemoslolin32 lite by using Arduino IDE, We tried to use the Adafruit Fingerprint library to upload code on Wemoslolin32 lite to detect the fingerprint sensor but it doesn't detect the finger sensor. Hence, we guess we have a problem with the Wemoslolin32 lite board.

## Report on 26 September 2567

![image](https://hackmd.io/_uploads/rkbZxh7CR.png)
![image](https://github.com/laleesaw/miniproject/blob/main/Screenshot%202024-09-27%20005850.png?raw=true)
![image](https://github.com/laleesaw/miniproject/blob/main/Screenshot%202024-09-27%20010306.png?raw=true)

After we found troubleshooting on 26 September, We decided to change the board from Wemoslolin32 lite to Uno R3 to fix a troubleshoot. Then when we uploaded it on the Uno R3 board, a board could detect a finger sensor and when we searched for a solution, we found that the voltage supply of Wemoslolin32 lite wasn't enough, finger sensor want 5 Voltage but the Wemoslolin32 lite could just give 3.3 Voltage. 






