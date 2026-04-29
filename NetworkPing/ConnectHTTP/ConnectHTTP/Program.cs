using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace ConnectHTTP
{
    class Program
    {
        static void Main(string[] args)
        {
            Uri uri = new Uri("http://192.168.1.130");
            //Uri uri = new Uri("http://www.seznam.cz");
            HttpWebRequest request = (HttpWebRequest)HttpWebRequest.Create(uri);
            request.Method = "GET";
            String test = String.Empty;
            using (HttpWebResponse response = (HttpWebResponse)request.GetResponse())
            {
                Stream dataStream = response.GetResponseStream();
                StreamReader reader = new StreamReader(dataStream);
                string line = reader.ReadLine();
                //test = reader.ReadToEnd();
                reader.Close();
                dataStream.Close();
             }
             
        }
    }
}
