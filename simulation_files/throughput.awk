BEGIN {
   init=0;
   i=0;
 }

{
   event = $1;
   time = $2;
   node_nb = $3;
   len=length(node_nb);
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
 if(len == 1)
{
    if(event=="r" && pkt_type=="cbr" )
    {
       pkt_byte_sum[i+1]=pkt_byte_sum[i] + (pkt_size-20);
       if(init == 0)
       {
          start_time = time;   #保存接收到第一个分组的时间
          init = 1;
       }
       end_time[i] = time;
       i++;
    }

  
}
}

 #END表明这是程序结束执行前的语句，也只执行一次
END {
   th = 8*pkt_byte_sum[i-1]/(end_time[i-1]-start_time)/1000;   #计算吞吐量 
   printf("%.2f\n",th);  
}

