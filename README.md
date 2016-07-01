# spider
crawl novel from qidian index

##Dependent Libraries
gumbo-query: used to parse html document. ->`libgq.so -> libgumbo.so` (Dependency relation)  
redis: act as database. ->`libhiredis.so`   

##install
make 

##run 
./spider -u www.qidian.com -t 1  
  
* -u :set the spider's root url.  
* -t : set the url's type.
    * 1  novel website index  //小说网站首页
    * 2  novel book index  //某小说首页
    * 3  novel book directory, //某小说目录
    * 4  novel book href,  //某小说链接
