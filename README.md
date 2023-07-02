
## JaguarDB is the Best Database For AI, IoT, Robots ##

**AI and IoT Data**

JaguarDB is extremely scalable, best suited for Artificial Intelligence and Internet of Things (IoT) which can handle
massive amounts of vector data, identity data, knowledge information, events data, location data, and time series data.
JaguarDB can scale out horizontally 10000 to 1 million times faster than any distributed systems that use
consistent-hashing algorithm. JaguarDB scales a large database system in seconds without 
interrupting your database system.  Imagine you will have million trillions of data elements, 
JaguarDB is perfect for your IT system.

**Instant Scaling**

In traditional way of horizontal scaling of distributed database systems, data migration is required and may
take a `LONG time`, referred as scaling nightmare. JaguarDB, with our unique Zero-Move Hash (ZMHash) technology,
does not require any data migration and can scale to thousands or millions of nodes
`INSTANTLY`, taking only a second or even less. 
JaguarDB scales the system by adding whole clusters where each cluster contains a volume of nodes.
Other database systems can only add a node one at a time. JaguarDB allows you to add thousands of nodes
 or more in one step. This is why JaguarDB can scale with lightning speed.


**AI Data**

Artificial intelligence (AI) systems are trained using large amounts of data to learn and improve their performance. 
This is because AI algorithms use statistical techniques to find patterns and make predictions based on the data 
they have been trained on. The more diverse and representative the data is, the better the AI will be able to 
learn and generalize from that data.

To create accurate and reliable AI models, it is important to ensure that the data used for training is of high quality, 
well-structured, and covers a wide range of scenarios and use cases. This allows the AI to learn from a 
variety of perspectives and make more accurate predictions or decisions when applied to new data.


```

       JaguarDB                          Model                            JaguarDB

                                                                     --------------------
    ----------------            --------------------------          | Generated Content  |
   | Training Data  |   ===>   |                          |  ====>  | Embedding Vectors  |  <===== Search
    ----------------           |    AI  Neural Network    |          --------------------
                               |                          |        
    ----------------           |         Text             |          --------------------    
   | Knowledge Base |   ===>   |         Audio            |  ====>  |   False Positives  |  ------------v
    ----------------           |         Video            |          --------------------               |
                               |        Images            |                                             |
    ----------------           |                          |          --------------------               |
   |  Dynamic Data  |   ===>   |       Embeddings         |  ====>  |  False Negatives   |  ------v     |
    ----------------            --------------------------           --------------------         |     |
                                       ^      ^                                                   |     |
                                       |      |                                                   |     |
                                       |      |                                                   |     |
       Achieving                       |      ^---------------------------------------------------<     |
                                       |                                                                |
      Realtime AI                      |                                                                |
                                       ^----------------------------------------------------------------<

```


Therefore, having lots of good data is essential for developing robust and accurate AI models that can be applied 
in a variety of contexts and provide value to businesses and individuals alike. Good data comes from a well-managed database
where knowledge and facts are maintained and fed to the AI systems to achieve another level of intelligence.


**Location Data**

JaguarDB stands out as the sole database that offers comprehensive support for both 
vector and raster spatial data. With JaguarDB, users can seamlessly work with a 
wide range of spatial shapes in their datasets.

For vector spatial data, JaguarDB supports an extensive set of shapes, including lines, 
squares, rectangles, circles, ellipses, triangles, spheres, ellipsoids, cones, 
cylinders, boxes, and their 3D counterparts. This broad range of vector shapes 
empowers users to accurately represent and analyze complex spatial structures in their data.
When it comes to raster spatial data, JaguarDB enables the handling of point data, 
multipoints, linestrings, multilinestrings, polygons, multipolygons, as well as their 3D equivalents. 
This comprehensive support for raster shapes allows for the efficient storage and analysis 
of geospatial information in various forms.

In addition to its versatile spatial data capabilities, JaguarDB surpasses other databases by 
offering support for unlimited tags at a given location. This feature proves invaluable when 
analyzing intricate location-based data and metrics. By allowing unlimited tags, JaguarDB 
enables users to associate rich and detailed information with specific locations, 
facilitating sophisticated analysis and in-depth insights.


**Time Series Data**

JaguarDB excels in facilitating rapid ingestion of time series data, including the integration 
of location-based data. Its unique capabilities extend to indexing in both spatial and temporal 
dimensions, enabling efficient data retrieval based on these critical aspects. Moreover, 
JaguarDB offers exceptional speed when it comes to back-filling time series data, 
allowing for the seamless insertion of large volumes of data into past time periods.

One of JaguarDB's standout features is its automatic data aggregation across multiple 
time windows. This functionality eliminates the need for additional computational work, 
as users can instantly access aggregated data without any extra effort. By providing 
immediate access to aggregated data, JaguarDB streamlines data analysis and empowers 
users to derive valuable insights without delays.


**File Storage and Search**

JaguarDB offers a versatile file storage solution that allows users to effortlessly upload various types of 
data files, including videos, photos, and other file formats, into their system. During the upload 
process, users have the option to attach keywords or tags to each file, facilitating easy and efficient 
retrieval in the future. With JaguarDB's advanced search capabilities, users can search through trillions 
of files using these keywords or tags, enabling them to find specific files quickly and effectively. 
This feature eliminates the arduous task of searching for a needle in a haystack and streamlines the 
entire process by a magnitude of millions.


## Source Code ##
This github account contains the source code for jaguardb server programs. To clone it:

```
  git clone https://github.com/fserv/jaguardb.git

```

## JaguarDB History ##

Our original repository was in https://github.com/datajaguar/jaguardb but due to accidental checkin of some large
binary packages, checking out the repository met some issues. The development of JaguarDB started in 2013 and 
the public repository was checked in github at 2017. After 10 years of development, JaguarDB now has reached milestones
in the scalability, feature set, high availability, and stability.

## Compiled Binary Package ##
You can build the binary programs from the github repository. However, since JaguarDB requires several
third-party packages and if you do not have time to build, you can directly visit our web site and just download
an already-built package for JaguarDB:

Go to this web page to download the compiled package:  [Download JaguarDB Package](http://www.jaguardb.com/download.php)


## Web Site

Our web site is at:   

    http://www.jaguardb.com

## Deployment 

JaguarDB has undergone a rigorous journey of over 300 releases and iterations, accompanied by an 
extensive testing process comprising 1421 test cases. As a result of this meticulous development and 
quality assurance effort, JaguarDB has achieved a high level of stability and reliability that makes 
it ideal for product environments. Prospective users can have full confidence in deploying JaguarDB in 
production settings, knowing that it has been thoroughly vetted and proven to deliver consistent 
performance and robust functionality. The extensive testing and continuous refinement of JaguarDB demonstrate 
a commitment to excellence, ensuring that it meets the stringent requirements of real-world scenarios 
and empowers organizations with a dependable and efficient data storage solution.
