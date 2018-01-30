import linecache;
import matplotlib.pyplot as plt;
import re;

ta = [];
tb = [];
rqa = [];
rqaa = [];

rqb = [];
rqbb = [];
tad = [];
tbd = [];
rqad = [];
rqbd = [];

pta = [];
prqa = [];

ptb = [];
prqb = [];


for line in open("./p2c/redQueueA.plot"):
    num = line.split();
    ta.append(float(num[0]));
    rqa.append(float(num[1]));

for line in open("./p2c/redQueueAAvg.plot"):
    num = line.split();
    rqaa.append(float(num[1]));

for line in open("./p2c/redQueueB.plot"):
    num = line.split();
    tb.append(float(num[0]));
    rqb.append(float(num[1]));

for line in open("./p2c/redQueueBAvg.plot"):
    num = line.split();
    rqbb.append(float(num[1]));

for line in open("./p2c/PacketNumA.plot"):
    num = line.split();
    pta.append(float(num[0]));
    prqa.append((int(num[1])-1)/958);

for line in open("./p2c/PacketDropA.plot"):
    num = line.split();
    tad.append(float(num[0]));
    rqad.append((int(num[1])-1)/958);


for line in open("./p2c/PacketDropB.plot"):
    num = line.split();
    tbd.append(float(num[0]));
    rqbd.append((int(num[1])-1)/958);   

rqad0 = [0]*len(tad);
rqbd0 = [0]*len(tbd);

plt.subplot(311);
plt.plot(ta,rqa,label='QueueSize');
plt.plot(ta,rqaa,'--',label='Average QueueSize');
plt.plot(tad, rqad0,'x',label='PacketDropSizeA');
plt.legend();
plt.ylabel('Queue for gate A');
plt.xlabel('Time');

plt.subplot(312);
plt.plot(tb,rqb,label='QueueSize');
plt.plot(tb,rqbb,'--',label='Average QueueSize');
plt.plot(tbd, rqbd0, 'x',label='PacketDropSizeB');
plt.legend();
plt.ylabel('Queue for gate B');
plt.xlabel('Time');

plt.subplot(313);
plt.plot(pta, prqa, '.',label='PacketNum');
plt.plot(tad, rqad, 'x',label='PacketDropA');
plt.plot(tbd, rqbd, 'x',label='PacketDropB');
plt.plot(tad, rqad0, 'x',label='PacketDropSizeA');
plt.plot(tbd, rqbd0, 'x',label='PacketDropSizeB');
plt.legend();
plt.ylabel('Packet Number For Each Connections');
plt.xlabel('Time');
plt.show();


        


