# Record pedal state logs
1) Open the Simhub plugin and connect to the pedal.
2) Select "Debug Flag 0" = 64 <br> <img width="200" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/63e11f6c-c68b-4288-bfad-b6d732668d14"> <br> This will tell the ESP to broadcast the extended pedal info. <br>
3) Send the config to the pedal.
4) Activate the "Log pedal states" switch <br> <img width="200" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/91138b5d-9903-4736-b46f-159eefd1b0e8"> <br>
5) The pedal state record can be found in you Simhub installation directory, e.g.:  <br> <img width="500" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/f32f461a-e1af-4a49-a92b-64daa4e745e4">. <br>
The file naming is "DiyFfbPedalStateLog_0: for clutch", "DiyFfbPedalStateLog_1: for brake" and "DiyFfbPedalStateLog_2: for throttle".
6) Now perform whatever test you want.
7) Deactivate "Log pedal states" again
8) Select "Debug Flag 0" = 0
9) Send the config to the pedal.

# Visualize the pedal logs
## Option 1: Via Google Colab:
1) Open this [link](https://colab.research.google.com/github/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/Validation/VisualizePedalLog.ipynb)
2) Upload the pedal log file, see <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/e4215739-4a6f-40ac-8531-cf2c7b5e2120"> <br>
3) Run the code <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/0183959a-20a7-4bb5-920b-bb88a95e3cb2"> <br>
4) Inspect the generated graphs, e.g. <br> <img width="980" alt="Bildschirmfoto 2024-11-01 um 20 48 59" src="https://github.com/user-attachments/assets/d8d3c60a-5d7f-44c2-a404-40fc3253edc1">


## Option 2: Via python fiddle
1) Open [jupyter online](https://python-fiddle.com/)
2) Upload the plotting code by opening "upload code context menu" <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/878bfa80-b024-4f1e-bb99-c0760aeb9418"> <br>
and inserting the [jupyther notebook](https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/Validation/VisualizePedalLog.ipynb) link <br>
<img width="400" alt="Bildschirmfoto 2024-11-01 um 20 37 07" src="https://github.com/user-attachments/assets/c6f1538d-eec2-43c0-b3dd-773b0bd40678">  <br>

3) Upload the pedal log file (e.g. "C:\Program Files (x86)\SimHub\PluginsData\Common\DiyFfbPedalStateLog_1.txt") <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 44 36" src="https://github.com/user-attachments/assets/2b86cc91-302d-4b39-808c-e154702d0e92">

4) Run the script <br> <img width="400" alt="Bildschirmfoto 2024-11-01 um 20 46 36" src="https://github.com/user-attachments/assets/76f00347-1ef6-413e-806a-3c908aa736a3">

5) Inspect the generated graphs, e.g. <br> <img width="980" alt="Bildschirmfoto 2024-11-01 um 20 48 59" src="https://github.com/user-attachments/assets/d8d3c60a-5d7f-44c2-a404-40fc3253edc1">
