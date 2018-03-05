% clear all
f0 = 100;
f1 = 10;
dur = 0.1;
ioidef = 0.5;
Fs = 44100;
t = (0:1000000)'/Fs;
n = 1;
x = zeros(size(t));
while n < length(t)
    x(n:n+dur*Fs-1) = sin(2*pi*linspace(f0,f1,dur*Fs)'.*t(1:dur*Fs));
    n = n + ioidef*Fs;
end
x = x(1:length(t));
y = x;
% either use the sound generated above, or load one:
[y, origFs] = audioread('~/Desktop/disco.wav');
Fs = origFs;
y = sum(y,2);
% plot(x)
% player = audioplayer(x, Fs);
% player.play

%%
q = 100;
[B, A] = butter(2, 300/(origFs/2));
x = filter(B, A, y);
Fs = origFs / q;
% block-based approach, not suitable for real-time
taus = srange(1):srange(2);
startT = max(taus)+1;
comb = zeros(length(taus), startT+N);
if(0)
for n = 1:length(taus)
    tau = taus(n);
    t0 = 1500/Fs;
    alpha = 0.5 ^ ((tau/Fs) / t0);
    for t = tau+1:startT+N
        comb(n,t) = alpha * comb(n, t - tau) + (1 - alpha) * x(t);
    end
end
end
out = sum(abs(comb').^2)./taus;
COMB = comb;
OUT = out;
plot(taus/Fs*60, out)
[~, m] = max(out);
taus(m)/Fs*60

%%
% sample-based approach, suitable for real-time
x = filter(B, A, y);
downsampledCount = 1;
downsampleRatio = q;
Fs = origFs / downsampleRatio;
idx = 1;
bufferSize = 128;
writePtr = 1;
comb = nan(length(taus), startT+N);
while idx + bufferSize < length(x)
    fa = x(idx:idx+bufferSize - 1);
    for n = 1 : length(fa)
        if downsampledCount == 1
            in = fa(n);
            for tauIndex = 1:length(taus)
                tau = taus(tauIndex);
                if writePtr - tau >= 1
                    t0 = 1500/Fs;
                    alpha = 0.5 ^ ((tau/Fs) / t0);
                    out = alpha * comb(tauIndex, writePtr - tau) + (1 - alpha) * in;
                else
                    out = 0;
                end
                comb(tauIndex,writePtr) = out;
            end
            writePtr = writePtr + 1;
            if writePtr == N + startT + 1
                % analyze, reset, restart
                out = sum(abs(comb').^2)./taus;
                plot(taus/Fs*60, out)
                [~, m] = max(out);
                taus(m)/Fs*60
                writePtr = 1;
            end
        end
        downsampledCount = downsampledCount + 1;
        if downsampledCount == downsampleRatio + 1
            downsampledCount = 1;
        end
    end
    idx = idx + bufferSize;
end

plot(out)