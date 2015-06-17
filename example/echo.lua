local recvcount = 0

C.Listen("127.0.0.1",8010,function (s)
	print("new client",s)
	C.Bind(s,C.PacketDecoder(),function (s,rpk)
		print("recv packet",rpk:ReadStr(),recvcount)
		recvcount = recvcount + 1;
		C.Send(s,C.NewWPacket(rpk))
	end)
end)

while true do
	C.Run(50)
end
