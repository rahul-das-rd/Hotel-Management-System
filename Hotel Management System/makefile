table: table.c
	gcc table.c -o table -lrt

waiter: waiter.c
	gcc waiter.c -o waiter -lrt

admin: admin.c
	gcc admin.c -o admin -lrt

hotelmanager: hotelmanager.c
	gcc hotelmanager.c -o hotelmanager -lrt

all: table waiter admin hotelmanager

tableclean: table
	rm table

waiterclean: waiter
	rm waiter

adminclean: admin
	rm admin

hotelmanagerclean: hotelmanager
	rm hotelmanager