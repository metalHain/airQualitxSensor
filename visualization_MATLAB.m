clear all; clc;

% read tab separated text file from air measuring unit and plot graphs for
% sensor values over time

% author Jonas Hain - student at Th Nürnberg Georg Simon Ohm- 2023

readData = readtable("dummyData.txt");

readData = table2array(readData);

% date vectors
yy = readData(:,7);
mm  = readData(:,6);
dd = readData(:,5);
hh = readData(:,2);
min = readData(:,3);
sec = readData(:,4);

% sensor values in vectors
temperature = readData(:, 8);
humidity = readData(:, 9);
co_conc = readData(:, 10);
co2_conc = readData(:,11);
batLev = readData(:,12);

datesFromReadDate = datetime(yy, mm, dd, hh, min, sec);


% plot graphs

figure('Name', 'Measured Data');

t = tiledlayout(2,2);

nexttile
plot(datesFromReadDate, temperature, "r")
ylim([-20, 40])
title("room temperature")
ylabel("temperature [°C]")
xlabel("time")

nexttile
plot(datesFromReadDate, humidity, "b")
title("room humidity")
ylabel("humidity [%]")
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


figure('Name', 'Battery Voltage');

plot(datesFromReadDate, batLev);
title("battery voltage");
ylabel("battery voltage [V]")
xlabel("time")


