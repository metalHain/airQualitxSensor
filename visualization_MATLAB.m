clear all; clc;
close all;

% read tab separated text file from air measuring unit and plot graphs for
% sensor values over time

% author Jonas Hain - student at Th Nürnberg Georg Simon Ohm- 2023
FID_1 = fopen('DATLOG.txt', 'r');
FID_2 = fopen('DATLOG_Wheader.txt', 'w');

fprintf(FID_2, "hh\tmin\tsec\tday\tmo\tyy\tco\tco2\ttemp\thumi\tbat");
writematrix(readmatrix('DATLOG.txt'), 'DATLOG_Wheader.txt', 'Delimiter', 'tab', 'WriteMode', 'append');

readData = tdfread("DATLOG_Wheader.txt", '\t');

% date vectors
yy = readData.yy;
mm  = readData.mo;
dd = readData.day;
hh = readData.hh;
minutes = readData.min;
sec = readData.sec;

% sensor values in vectors
temperature = readData.temp;
humidity = readData.humi;
co_conc = readData.co;
co2_conc = readData.co2;
batLev = readData.bat;

datesFromReadDate = datetime(yy, mm, dd, hh, minutes, sec);

% median - min - max calculations

Statistics = table(temperature, humidity, co_conc, co2_conc, batLev);

% plot graphs

figure('Name', 'Measured Data');

t = tiledlayout(2,2);

nexttile
plot(datesFromReadDate, temperature, "r")
title("room temperature")
ylabel("temperature [°C]")
ylim([0,30])
xlabel("time")

nexttile
plot(datesFromReadDate, humidity, "b")
title("room humidity")
ylabel("humidity [%]")
ylim([50,80])
xlabel("time")

nexttile
plot(datesFromReadDate, co_conc, "g")
title("CO concentration")
ylabel("CO concentration [ppm]")
xlabel("time")


nexttile
plot(datesFromReadDate, co2_conc, "m")
title("CO2 concentration")
ylabel("CO2 concentration [ppm]")
xlabel("time")

figure('Name', 'Battery discharge Curve')
plot(datesFromReadDate, batLev, "b")
title("Battery voltage")
ylabel("level [%]")
ylim([0,100])
xlabel("time")


summary(Statistics);

