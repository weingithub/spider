# spider
crawl novel from qidian index

##Dependent Libraries
gumbo-query: used to parse html document. ->libgq.so -> libgumbo.so (Dependency relation)  
redis: act as database. ->libhiredis.so   

##install
make 

##run 
./spider -u www.qidian.com -t 1  
  
-u :set the spider's root url.  
-t : set the url's type
