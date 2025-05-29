# def set_breakdown():
#     lines = [""] * 4
#     trace = list()
#     # print(type(lines))

#     fetch_to_dram = 0  # s1 - s2 
#     process_command = 0 # s2 - s3 
#     mv_to_nvm = 0 # s3 
#     reply_to_client = 0 # s4 

#     with open("set_latency.log", 'r', encoding='utf-8') as file:
#         s1, s2, s3, s4 = 0, 0, 0, 0
#         for line in file:
#             trc = line.strip() 

#             if "s1" in trc:
#                 time = int(trc.split()[2])
#                 s1 += time
#             elif "s2" in trc:
#                 time = int(trc.split()[2])
#                 s2 = time
#             elif "s3" in trc:
#                 time = int(trc.split()[2])
#                 s3 = time
#             elif "s4" in trc:
#                 time = int(trc.split()[2])
#                 s4 = time
                
#                 mv_to_nvm += s3 
#                 process_command += (s2-s3)
#                 fetch_to_dram += (s1-s2)
#                 reply_to_client += s4
                
#                 s1, s2, s3, s4 = 0, 0, 0, 0

def split(filename,set_fname, get_fname):
    
    try:
        set_file = open(set_fname, 'w', encoding='utf-8')
        get_file = open(get_fname, 'w', encoding='utf-8')
        
        set_file.write("set_file\n")
        get_file.write("get_file\n")

    except Exception as e:
        print(str(e))
    
    with open(filename, 'r', encoding='utf-8') as file:
        n = 0 
        br1, br2 = 0, 0
        trc=""
        for line in file:
            prev_trc = trc
            trc = line.strip()
            # print(trc)
            if "Ready to accept" in trc:
                br1=n
                print(n)            

            if "setCommand" in trc:
                br2=n+2
                # print(n)
            n+=1
    
    # print("breakline = " + str(br1) + " " + str(br2))
    
    with open(filename, 'r', encoding='utf-8') as file:
        n = 0
        for line in file:
            trc = line.strip() 
            # print(n, trc)
            if n >= br1 and n < br2 and "SIGINT" not in trc:
                set_file.write(trc+"\n")
            elif n >= br2 and "SIGINT" not in trc:
                get_file.write(trc+"\n")
                
            n+=1
    
    set_file.close()
    get_file.close()
                
def lat_breakdown(filename):
    print("\n" + filename + " ...")
    fetch_to_dram = 0  # s1 - s2 
    process_command = 0 # s2 - s3 
    mv_to_nvm = 0 # s3 
    reply_to_client = 0 # s4 

    ops=60000

    n = 0
    with open(filename, 'r', encoding='utf-8') as file:
        s1, s2, s3, s4 = 0, 0, 0, 0
        for line in file:
            trc = line.strip() 
            n += 1

            if "s1-readQueryFromClient:" in trc:
                time = int(trc.split()[2])
                # s1 += time
                fetch_to_dram += time
            elif "s2-processCommand:" in trc:
                time = int(trc.split()[2])
                # s2 = time
                process_command += time

            elif "s3-sdsmvtonvm:" in trc:
                time = int(trc.split()[2])
                # s3 = time
                mv_to_nvm += time
                # print("s3-sdsmvtonvm", time)
            elif "s4" in trc:
                # print(n, trc.split())
                time = int(trc.split()[2])
                # s4 = time
                reply_to_client += time
                
                # mv_to_nvm += s3 
                # process_command += (s2-s3)
                # process_command += s2
                # fetch_to_dram += (s1-s2)
                # fetch_to_dram += s1
                # reply_to_client += s4
                
    # s1, s2, s3, s4 = 0, 0, 0, 0

    rfilename = filename.replace(".log", ".txt")
    # print(rfilename)
    with open(rfilename, 'w', encoding='utf-8') as wfile:
        wfile.write("1-fetch-to-dram: " + str(fetch_to_dram/ops) + "\n")
        wfile.write("2-process-command: " + str((process_command - mv_to_nvm)/ops) + "\n")
        wfile.write("3-mv-to-nvm: " + str(mv_to_nvm/ops) + "\n")
        wfile.write("4-reply-to-client: " + str(reply_to_client/ops) + "\n")

    # print("s1-fetch_to_dram: ", fetch_to_dram/ops)
    # print("s2-process_command: ", (process_command - mv_to_nvm)/ops)
    # print("s3-mv_to_nvm: ", mv_to_nvm/ops)
    # print("s4-reply_to_client: ", reply_to_client/ops)

    
def get_time(set_fname):
    with open(set_fname, 'r', encoding='utf-8') as file:
        for line in file:
            trc = line.strip()
            
            if "s3-sdsmvtonvm" in trc:
                s3=int(trc.split()[2])
            if "s2-processCommand" in trc:
                s2=int(trc.split()[2])
                print(s2-s3)


if __name__ == "__main__":
    # get_time("set_pmem_latency.log")
    
    dir="./backups_64KB_230107/"
    # dir="./backups_64KB_final/"
    dir="./"
    # log_files=["redis_dram.log", "redis_pmem_sp.log", "redis_pmem_asp.log", "redis_pmem_np.log"]
    #log_files=["redis_bpf.log"]
    log_files=["redis_dram.log", "redis_pmem.log", "redis_bpf.log"]    
    for lfile in log_files:
        tot_fname = dir+lfile
        set_fname = dir+lfile.replace("redis_", "set_")
        get_fname = dir+lfile.replace("redis_", "get_")
        split(tot_fname, set_fname, get_fname)
        # print(tot_fname, set_fname, get_fname)
        lat_breakdown(set_fname)
        lat_breakdown(get_fname)
    # # tot_fname="./backups_64KB_final/redis_dram.log"
    # # tot_fname="./redis_dram.log"
    # # tot_fname="redis_dram_th4_local.log"    
    # set_fname = "set_dram_latency.log"
    # get_fname = "get_dram_latency.log"
    # split(tot_fname, set_fname, get_fname)
    # lat_breakdown(set_fname)    
    # lat_breakdown(get_fname)
    
    # tot_fname="./backups_64KB_final/redis_pmem.log"
    # # tot_fname="./redis_pmem.log"
    # # tot_fname="redis_pmem_th4_local.log"    
    # set_fname = "set_pmem_latency.log"
    # get_fname = "get_pmem_latency.log"
    # split(tot_fname, set_fname, get_fname)
    # lat_breakdown(set_fname)    
    # lat_breakdown(get_fname)
    
# with open("set_latency.log", 'r', encoding='utf-8') as file:
#     n=0
#     for line in file:
#         idx = n%4
#         # print(line.strip())
#         lines[idx] = line.strip()
#         # print(lines[idx][0])
        
#         if "s4" in lines[idx]:
#             for x in range(3,-1,-1):
#                 trace.append(lines[(n-x)%4].strip())
#                 # print(lines[(n-x)%4].strip())
#         n += 1
                
#     # print(len(trace))
#     # for trc in trace:
#     #     print(trc)        
        
#     n=0
#     while n < len(trace):
#         s3=int(trace[n].split()[2])
#         s2=int(trace[n+1].split()[2])
#         s1=int(trace[n+2].split()[2])
#         s4=int(trace[n+3].split()[2])
        
#         # print(s3, s2, s1, s4)
#         mv_to_nvm += s3 
#         process_command += (s2-s3)
#         fetch_to_dram += (s1-s2)
#         reply_to_client += s4
#         n += 4


