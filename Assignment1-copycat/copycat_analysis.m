% Elie Weintraub 
% OS - Assignment 1: copycat_analysis.m
clc, clear all, close all,

%% Define data
file_size=3931199*2^-20;             %filesize in MB
buffer_size=2.^(0:18)';             %buffer size in bytes
time  =  [17.844  0.677  17.080;    %time: [ real user sys ]
           8.978  0.367   8.543;
           4.563  0.147   4.360;
           2.342  0.097   2.137;
           1.169  0.040   1.087;
           0.624  0.017   0.563;
           0.339  0.010   0.290;
           0.205  0.007   0.160;
           0.137  0.007   0.087;
           0.089  0.000   0.043;
           0.084  0.003   0.023;
           0.083  0.000   0.023;
           0.657  0.000   0.023;
           0.062  0.000   0.020;
           0.067  0.000   0.037;
           0.064  0.003   0.033;
           0.247  0.000   0.027;
           0.179  0.000   0.030;
           0.201  0.000   0.027];

time2  =  [17.766  0.750  16.920;
	        8.929  0.333   8.540;
	        4.579  0.217   4.310;
            2.291  0.130   2.120;
            1.168  0.060   1.063;
            0.621  0.040   0.533;
            0.332  0.010   0.283;
            0.200  0.000   0.157;
            0.135  0.000   0.087;
            0.093  0.010   0.043;
            0.073  0.000   0.030;
            0.065  0.003   0.017;
            0.062  0.000   0.020;
            0.065  0.000   0.020;
            0.059  0.007   0.007;
            0.055  0.000   0.013;
            0.057  0.000   0.017;
            0.066  0.000   0.020;
            0.057  0.003   0.017];

%% Plots of throughput       
time=[time(:,1) time(:,2)+time(:,3)];     %time [ real cpu]
time2=[time2(:,1) time2(:,2)+time2(:,3)]; %time [ real cpu]
figure('Name','Copycat Throughput Analysis ');
subplot(311),plot(0:18,file_size./time,'-x');       
title('Copycat Throughput Analysis 1');
xlabel('buffer size [log_2(Bytes)]'), ylabel('throughput [MB/sec]');
legend('real time', 'cpu time');
subplot(312),plot(0:18,file_size./time2,'-x');       
title('Copycat Throughput Analysis 2');
xlabel('buffer size [log_2(Bytes)]'), ylabel('throughput [MB/sec]');
legend('real time', 'cpu time');
subplot(313),plot(0:18,file_size./time2(:,1),'-x');       
title('Copycat Throughput Analysis 2');
xlabel('buffer size [log_2(Bytes)]'), ylabel('throughput [MB/sec]');
legend('real time');

%% Final plot
figure('Name','Copycat Throughput Analysis ');
plot(0:18,file_size./time2(:,1),'-x');       
title('Copycat Throughput Analysis ');ylim([0 80]);
xlabel('buffer size [log_2(Bytes)]'), ylabel('throughput [MB/sec]');
legend('real time');