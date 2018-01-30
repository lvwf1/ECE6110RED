import linecache;
import matplotlib.pyplot as plt;
import re;
t1 = [];
rq = [];
rq_avg = [];

t2 = [];
pn = [];

t3 = [];
pd = [];
for line in open("./p2a/redQueue.plot"):
    num = line.split();
    t1.append(float(num[0]));
    rq.append(float(num[1]));

for line in open("./p2a/redQueueAvg.plot"):
    num = line.split();
    rq_avg.append(float(num[1]));

for line in open("./p2a/PacketNum.plot"):
    num = line.split();
    t2.append(float(num[0]));
    pn.append(int(int(num[1])/958)%90+(int(num[2])-8081)*100);

for line in open("./p2a/PacketDrop.plot"):
    num = line.split();
    t3.append(float(num[0]));
    if len(num) < 3:
        pd.append(0);
    else:
        pd.append(int(num[1])%90 + int(num[2]-1)*100);
    

plt.subplot(211);
plt.plot(t1,rq,label='Queue Size');
plt.plot(t1,rq_avg,'--',label='Average Queue Size');
plt.legend();
plt.ylabel('Queue');
plt.xlabel('Time');

plt.subplot(212);
plt.plot(t2, pn, '.', label='PacketNum')
plt.plot(t3, pd, 'x', label='PacketDrop');
plt.ylabel('Packet Number(mod 90) For Four Connections');
plt.xlabel('Time');
plt.yticks([0,100,200,300,400]);
plt.hlines(100, 0, 1, linestyles = "dashed");
plt.hlines(200, 0, 1, linestyles = "dashed");
plt.hlines(300, 0, 1, linestyles = "dashed");
plt.legend()
plt.show();
