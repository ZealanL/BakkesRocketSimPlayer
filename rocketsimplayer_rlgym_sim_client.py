import socket
import struct
	
from rlgym_sim.utils.gamestates import GameState

UDP_IP = "127.0.0.1"
UDP_PORT = 4577

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP

def pack_vec(vec):
	return struct.pack("<f", vec[0]) + struct.pack("<f", vec[1]) + struct.pack("<f", vec[2])

def pack_physobj(physobj):
	forward = physobj.forward()
	right = physobj.right()
	up = physobj.up()
	return (
		pack_vec(physobj.position) + 
		pack_vec(forward) + pack_vec(right) + pack_vec(up) + 
		pack_vec(physobj.linear_velocity) + 
		pack_vec(physobj.angular_velocity)
		)

def pack_car(player, action):
	bytes = b""
	
	# Team number
	bytes += struct.pack("B", int(player.team_num))
	
	# Car physics state
	bytes += pack_physobj(player.car_data)
	
	# Car boost
	bytes += struct.pack("<f", player.boost_amount)
	
	# Car state bools
	bytes += struct.pack("B", player.on_ground)
	bytes += struct.pack("B", player.has_jump)
	bytes += struct.pack("B", player.has_flip)
	bytes += struct.pack("B", player.is_demoed)

	for i in range(8):
		bytes += struct.pack("<f", action[i])
	
	return bytes

def send_data(gs: GameState, actions):
	msg = b""

	tick_count = 0 # Not implemented yet
	msg += struct.pack("I", tick_count)

	# Send cars
	msg += struct.pack("I", len(gs.players))
	i = 0
	for player in gs.players:
		if (len(actions) % 9) == 0:
			action = actions[i*9:(i+1)*9]
			action = action[1:]
		elif (len(actions) % 8) == 0:
			action = actions[i*8:(i+1)*8]
		else:
			action = [0, 0, 0, 0, 0, 0, 0, 0]
			print("BakkesRocketSimPlayer send_data(): Unknown action format, actions should be a 1D array of floats, 8 or 9 per player")
		msg += pack_car(player, action)
		i += 1
		
	# Send ball
	msg += pack_physobj(gs.ball)

	sock.sendto(msg, (UDP_IP, UDP_PORT))