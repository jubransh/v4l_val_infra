import logging
import time
import os
import csv
import paramiko

paramiko.util.log_to_file("paramiko.log")

time_stamp = time.strftime("%Y-%m-%d--%H-%M-%S")

hosts = "d457jetson03.iil.intel.com,d457jetson05.iil.intel.com,d457jetson06.iil.intel.com,d457jetson08.iil.intel.com"
# hosts = "d457jetson08.iil.intel.com,d457jetson07.iil.intel.com,d457jetson06.iil.intel.com,d457jetson05.iil.intel.com,d457jetson03.iil.intel.com,d457jetson01.iil.intel.com"
host_list=hosts.split(",")
orig_logs_folder="/home/administrator/Logs/"
ssd_logs_folder="/media/administrator/DataUSB/storage/Logs/"

# collect_raw_data=True
collect_raw_data=False
# collect_log_file = True
collect_log_file = False

target_folder = r"c:\log"
download_folder=os.path.join(r"c:\D457-temp",time_stamp)


def get_file_from_host(host,target_folder):
    logger.info("connecting to :{}".format(host))
    transport = paramiko.Transport((host,22))
    transport.connect(None,"administrator","trio_012")
    sftp = paramiko.SFTPClient.from_transport(transport)
    try:
        sftp.chdir(ssd_logs_folder)
        logs_folder=ssd_logs_folder
    except IOError as e:
        logs_folder=orig_logs_folder
    logger.info("Log Folder : " + logs_folder + " in host: " + host)
    sids=sftp.listdir(logs_folder)
    logger.info("going over SIDs")
    for sid in sids:
                logger.info("Found SID: "+sid+" in host: "+host)
                tests = sftp.listdir(logs_folder + "/" + sid)
                if not os.path.exists(os.path.join(target_folder,host)):
                    try:
                        os.makedirs(os.path.join(target_folder,host))
                    except:
                        pass
                if "tests_results.csv" in sftp.listdir(logs_folder + "/" + sid + "/" ):
                    logger.info("Found tests_results.csv in "+sid)
                    if not os.path.exists(os.path.join(target_folder, host,sid)):
                        try:
                            os.makedirs(os.path.join(target_folder, host,sid))
                        except:
                            pass
                    sftp.get(logs_folder + "/" + sid + "/tests_results.csv",os.path.join(target_folder,host,sid,"tests_results.csv"))
                else:
                    logger.error("tests_results.csv not found in SID: "+sid+ " in host: "+host)

                for test in tests:
                    if test != "tests_results.csv":
                        if "iteration_summary.csv" in sftp.listdir(logs_folder + "/" + sid + "/" + test):
                            if not os.path.exists(os.path.join(target_folder, host, sid,test)):
                                try:
                                    os.makedirs(os.path.join(target_folder, host, sid,test))
                                except:
                                    pass
                            logger.info("getting iteration_summary.csv from Test:"+test+" from SID:"+sid+" from host:"+ host)
                            sftp.get(logs_folder + "/" + sid + "/"+test+"/iteration_summary.csv",
                                     os.path.join(target_folder, host, sid,test, "iteration_summary.csv"))
                        else:
                            logger.error("iteration_summary.csv not found in test: "+test+" SID: "+sid+ " in host: "+host)
                        if collect_raw_data:
                          
                            if not os.path.exists(os.path.join(target_folder, host, sid, test)):
                                try:
                                    os.makedirs(os.path.join(target_folder, host, sid, test))
                                except:
                                    pass
                            for raw in sftp.listdir(logs_folder + "/" + sid + "/" + test):
                                if "raw_data" in raw:
                                    logger.info(
                                        "getting "+raw+" from Test:" + test + " from SID:" + sid + " from host:" + host)
                                    sftp.get(logs_folder + "/" + sid + "/" + test + "/"+raw,
                                             os.path.join(target_folder, host, sid, test, raw))

                        if collect_log_file:
                            if "test.log" in sftp.listdir(logs_folder + "/" + sid + "/" + test):
                                if not os.path.exists(os.path.join(target_folder, host, sid, test)):
                                    try:
                                        os.makedirs(os.path.join(target_folder, host, sid, test))
                                    except:
                                        pass
                                logger.info(
                                    "getting test.log from Test:" + test + " from SID:" + sid + " from host:" + host)
                                sftp.get(logs_folder + "/" + sid + "/" + test + "/test.log",
                                         os.path.join(target_folder, host, sid, test, "test.log"))
                            else:
                                logger.error(
                                    "test.log not found in test: " + test + " SID: " + sid + " in host: " + host)

                        if "result.csv" in sftp.listdir(logs_folder + "/" + sid + "/" + test):
                            if not os.path.exists(os.path.join(target_folder, host, sid, test)):
                                try:
                                    os.makedirs(os.path.join(target_folder, host, sid, test))
                                except:
                                    pass
                            logger.info(
                                "getting result.csv from Test:" + test + " from SID:" + sid + " from host:" + host)
                            sftp.get(logs_folder + "/" + sid + "/" + test + "/result.csv",
                                     os.path.join(target_folder, host, sid, test, "result.csv"))
                        else:
                            logger.error(
                                "result.csv not found in test: " + test + " SID: " + sid + " in host: " + host)

    if sftp: sftp.close()
    if transport: transport.close()


logger = logging.getLogger('fw_upgrade')
logger.setLevel(logging.DEBUG)
time_format = '%d/%m/%Y %H:%M:%S'
formatter = logging.Formatter('%(threadName)s %(asctime)s.%(msecs)03d %(levelname)s [%(filename)s,'
                              ' %(funcName)s(), Line %(lineno)d]: %(message)s', time_format)
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
ch.setFormatter(formatter)
logger.addHandler(ch)

if __name__ == '__main__':

    if not os.path.exists(download_folder):
        os.makedirs(download_folder)
    if not os.path.exists(target_folder):
        os.makedirs(target_folder)
    logfile =os.path.join(target_folder,time_stamp + "_log.log")
    fh = logging.FileHandler(logfile)
    fh.setLevel(logging.DEBUG)
    fh.setFormatter(formatter)
    logger.addHandler(fh)

    for host in host_list:
        get_file_from_host(host,download_folder)
    # download_folder = r"C:\Logs-D457"
    # intialize "tests_results.csv" file
    tests_results = open(os.path.join(target_folder,time_stamp+"_tests_results.csv"), "w+", newline='')
    tests_results_writer = csv.writer(tests_results)
    tests_header_added=False

    # intialize "iteration_summary.csv" file
    iteration_summary = open(os.path.join(target_folder,time_stamp+"_iteration_summary.csv"), "w+", newline='')
    iteration_summary_writer = csv.writer(iteration_summary)
    iterations_header_added=False

    # intialize "backup_results.csv" file - including unfinished tests
    backup_tests_results = open(os.path.join(target_folder, time_stamp + "_tests_results_fromIterations.csv"), "w+", newline='')
    backup_tests_results_writer = csv.writer(backup_tests_results)
    backup_tests_results_writer.writerow(["IP","Camera Serial","SID","Test name","Test Suite","Total iterations","Failed iterations","Pass rate"])

    # go over hosts in logs folder
    for host in os.listdir(download_folder):
        logger.info("found host:"+host)
        # go over SIDs in each host
        for sid in os.listdir(os.path.join(download_folder,host)):
            logger.info("Found SID: "+sid+" in host: "+host)
            # check if "tests_results.csv" file exist, if exits copy data to tests_results.csv
            if not os.path.exists(os.path.join(download_folder,host,sid,"tests_results.csv")):
                logger.error("tests_results.csv file is missing in:"+ os.path.join(download_folder,host,sid))
            else:
                with open(os.path.join(download_folder,host,sid,"tests_results.csv")) as csv_file:
                    csv_reader = csv.reader(csv_file, delimiter=',')
                    logger.info("Adding data from:"+os.path.join(download_folder,host,sid,"tests_results.csv"))
                    for index, row in enumerate(csv_reader):
                        if index == 0:
                            if not tests_header_added:
                                tests_header_added=True
                                tests_results_writer.writerow(row)
                        else:
                            tests_results_writer.writerow(row)
                    tests_results.flush()
            # go over tests in each SID
            for test in os.listdir(os.path.join(download_folder,host,sid)):
                logger.info("Found test: " + test +" in SID: "+sid + " in host: " + host)
                if not os.path.exists(os.path.join(download_folder, host, sid,test, "iteration_summary.csv")):
                    logger.error("iteration_summary.csv file is missing in:" + os.path.join(download_folder, host, sid,test))
                else:
                    with open(os.path.join(download_folder, host, sid,test, "iteration_summary.csv")) as csv_file:
                        csv_reader = csv.reader(csv_file, delimiter=',')
                        logger.info("Adding data from:"+os.path.join(download_folder, host, sid,test, "iteration_summary.csv"))
                        for index, row in enumerate(csv_reader):
                            if index == 0:
                                if not iterations_header_added:
                                    iterations_header_added=True
                                    iteration_summary_writer.writerow(row)
                            else:
                                iteration_summary_writer.writerow(row)
                        iteration_summary.flush()

                # adding backup_test_results - go over the result csv file and get the test name, total iterations and failed iterations
                test_name=test
                testSuite=""
                cameraSerial=""
                curr_iteration = -1
                total_iterations = 0
                Failed_iterations = 0
                if not os.path.exists(os.path.join(download_folder, host, sid, test, "result.csv")):
                    logger.error(
                        "result.csv file is missing in:" + os.path.join(download_folder, host, sid, test))
                else:
                    with open(os.path.join(download_folder, host, sid, test, "result.csv")) as csv_file:
                        csv_reader = csv.DictReader(csv_file, delimiter=',')
                        logger.info("Adding data from:" + os.path.join(download_folder, host, sid, test,
                                                                       "result.csv"))
                        line_count = 0
                        for row in csv_reader:
                            if line_count == 0:
                                line_count +=1
                            else:
                                line_count+=1
                                if curr_iteration!= row["Iteration"]:
                                    testSuite=row[" Test Suite"]
                                    cameraSerial = row[" Camera Serial"]
                                    curr_iteration = row["Iteration"]
                                    total_iterations+=1
                                    if row["Iteration status"]=="Fail":
                                        Failed_iterations+=1
                    if total_iterations != 0:
                        backup_tests_results_writer.writerow([host, cameraSerial,sid, test,testSuite, total_iterations, Failed_iterations, 100*(total_iterations-Failed_iterations)/total_iterations])


                    backup_tests_results.flush()


    tests_results.close()
    iteration_summary.close()
    backup_tests_results.close()





