#!/usr/bin/python3

import socket, logging, struct, time
import matplotlib.pyplot as plt
import numpy as np



# without the last digits after the last point for broadcast
BROADCAST_NET = '192.168.0.'

# broadcast port
BROADCAST_PORT = 9999

# get multicast ip from esp8266 over udp connection.
#MCAST_GRP = '239.0.0.57'
#MCAST_PORT = 12345

helloString = 'wifi-serial:\r\n'
receiveBuffersize = 1024


## @brief Transmit string as UDB broadcast package over given Network.
# @param data Data as python3 string.
# @return None or response as bytearray.
def transmitBroadcastUDP(data):
	global receiveBuffersize
	
	# create socket
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
	
	# set socket broadcast options
	s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, True)
	
	# send data to socket
	s.sendto(data.encode(), (BROADCAST_NET + '255', BROADCAST_PORT))
	
	# try to get response
	re = s.recv(receiveBuffersize)
	
	# close socket
	s.close()
	
	if (re):
		logging.debug('Response after UDP broadcast transmission: %s', str(re))
		
		return re
	# end if
	
	return None
# end transmitBroadcastUDP


def getInfoOnBroadcastRequest():
	global helloString
	
	re = transmitBroadcastUDP(helloString)

	if (re):
		re = re.decode()
		
		logging.debug("data: %s", re.strip())
	else:
		logging.error('No bytes received.')
	# end if
	
	return re.split(':')
# end getInfoOnBroadcastRequest


def multicastClient(mc_group, mc_port):
	logging.debug('start multicast client')
	
	
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
	sock.bind((mc_group, mc_port))  # use MCAST_GRP instead of '' to listen only
	
	mreq = struct.pack("4sl", socket.inet_aton(mc_group), socket.INADDR_ANY)
	sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
	
	#buff_size = 256000
	#data = np.zeros(buff_size, np.uint8)
	#counter = 0
	
	start_time = time.time()
	
	while True:
		recv = sock.recv(receiveBuffersize)
		l = len(recv)
	
		logging.debug(recv)
		#print(recv.decode().strip())
		
		#logging.debug("size: %i", l)
		#if ((counter + l) > buff_size):
		#	break
		# end if
	
		#data[counter:counter + l] = np.frombuffer(recv, np.uint8)
		#counter += l
		
		#logging.debug("counter: %i", counter)
		
		#if (counter == buff_size):
		#	break
		# end if
		
		time.sleep(0.001)
	# end while
	
	#end_time = time.time()
	
	sock.close()
	
	#logging.debug("counter: %i", counter)
	#logging.debug("%f seconds", end_time - start_time)
	#logging.debug("transfer rate: %f Byte/s", counter / (end_time - start_time))
	
	#return data
# end multicastClient


def sendToServer(ip, port, msg):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.connect((ip, port))
	
	# send data
	txData = str(msg) + '\r\n'
	s.send(txData.encode());
	
	# get response
	rxData = s.recv(1024)
	s.close();
	
	rxData = rxData.decode()
	
	# debug output
	logging.debug("send: %s - receive: %s", txData, rxData)
# end sendToServer



def main():
	try:
		data = getInfoOnBroadcastRequest()
	except socket.timeout as t:
		logging.error('broadcast request: %s', t)
		return
	# end try
	
	# convert data to port and ip
	try:
		cl_ip = data[0]
		cl_port = int(data[1])
		mc_group = data[2]
		mc_port = int(data[3])
	except:
		logging.error('data (%s) have wrong format', data)
	# end try
	
	logging.info("client is on: %s:%i", cl_ip, cl_port)


	try:
		# The controller will be send this to UART.
		sendToServer(cl_ip, cl_port, 'hallo')
		
		# Try to set ssid
		sendToServer(cl_ip, cl_port, '+++esp-set-ssid:WLANName')
		
		# Try to set password
		sendToServer(cl_ip, cl_port, '+++esp-set-pass:Geheimespasswort')
		
		# Reset controller
		sendToServer(cl_ip, cl_port, '+++esp-reset:')
	except socket.timeout as t:
		logging.error('tcp transmit: %s', t)
	# end try

	
	
	# start multicast client
	try:
		data = multicastClient(mc_group, mc_port)
	except socket.timeout as t:
		logging.error('multicast request: %s', t)
	# end try

	
	#diff_data = np.diff(data, n = 1)
	#plt.close('all')
	#f, ax = plt.subplots(2, sharex=True)
	#ax[0].plot(data)
	#ax[1].plot(diff_data)
	#plt.show()
# end main


if __name__ == "__main__":
	logging.basicConfig(level=logging.DEBUG)
	
	try:
		main()
	except KeyboardInterrupt:
		logging.info('abort from user')
	# end try
# end if