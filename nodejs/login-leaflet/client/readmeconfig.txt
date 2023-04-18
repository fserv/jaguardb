
Instructions:

1. npm install

2.  Change the port settings for your environment:

Edit package.json file:

  a. Change the port number 3000 to your own port 
      "start": "PORT=3000 react-scripts start",

  b.  Change http port 8810 to your own port
     "proxy": "http://localhost:8810"

3. npm start

4. point your brower (chrom) to http://IP:8810
