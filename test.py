import os
import commands

commands.getstatusoutput('./pc1 > pc_1.txt')

for question in [2,3,4]:
        for thread in [1,2,4,8,16,32]:
                exec_pc = "./pc%d" %question
                output = "pc_%d_%d.txt" %(question, thread)
                if question == 3:
                        commands.getstatusoutput('%s %d 0 > %s' %(exec_pc, thread, output))
                else:
                        commands.getstatusoutput('%s %d > %s' %(exec_pc, thread, output))


