BEGIN {
   fsDrops=0;
   numfs2=0;
   numfs0=0;
}

{
   event = $1;
   time = $2;
   node_nb = $3;
   len = length(node_nb);
   if(len == 3 || len==4)
   {
      node_nb1=substr(node_nb,2,1);
      node_nb2=substr(node_nb,2,1);
      trace_type = $4;
      flag = $5;
      uid = $6;
      pkt_type = $7;
      pkt_size = $8;
   }
   else
  {
       from_node = $3;
       to_node = $4;
       pkt_type = $5;
       pkt_size = $6;
       flag = $7;
       uid = $12;
  }
 if(len==3 || len==4)
 {
   if(event=="s" && trace_type=="AGT" && pkt_type=="cbr" && node_nb1==8)
       numfs2++;
 }
else{
   if((event=="r") && (pkt_type=="cbr")&&(to_node==2 || to_node==1))
  {
         numfs0++;
  }
}   
}

 #END表明这是程序结束执行前的语句，也只执行一次
END {
loss_rate = 0;
fsDrops=numfs2-numfs0;        #计算丢失的分组数目
#loss_rate=fsDrops/numfs2;     
printf("reacive:%.3f\n",numfs0);#在相同的发送场景下，计算接收率

}

  














