using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Http;
using System.Net;


namespace HTTP_Test
{
    class program
    {
        static ulong mCounter;

        static void Main()
        {
            mCounter = 0;
            while (true)
            {
                mCounter++;
                Task t = new Task(HTTP_GET);
                t.Start();
                System.Threading.Thread.Sleep(1000);
            }
            //Console.ReadLine();
        }

        static async void HTTP_GET()
        {
            //var TARGETURL = "http://en.wikipedia.org/";
            var TARGETURL = "http://192.168.1.131";

            HttpClientHandler handler = new HttpClientHandler()
            {
                
            };


            //Console.WriteLine("GET: + " + TARGETURL);

            try
            {
                int a = 5;
                a <<= 8;
                // ... Use HttpClient.            
                HttpClient client = new HttpClient(handler);

                var byteArray = Encoding.ASCII.GetBytes("username:password1234");
                client.DefaultRequestHeaders.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Basic", Convert.ToBase64String(byteArray));

                HttpResponseMessage response = await client.GetAsync(TARGETURL);
                HttpContent content = response.Content;

                DateTime now = DateTime.Now;
                // ... Check Status Code                                
                Console.WriteLine(string.Format("{2} - {0} - Response StatusCode: {1}", mCounter, (int)response.StatusCode, now.ToString()));

                // ... Read the string.
                string result = await content.ReadAsStringAsync();


                Console.WriteLine(result);
            }
            catch (Exception e)
            {
             

                Console.WriteLine(e.ToString());
            }
            
        }
    }
}