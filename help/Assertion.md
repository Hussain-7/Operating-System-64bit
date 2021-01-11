
## Why we need Assert Function ##
Initially we assume that everything in it should run as we expect.
whereas small error will bring the whole system down.
The user programs are in the untrusted zone.When they send requests to the kernel by system call, the barricade will check those requests and data
to see if they are valid.
If not, we will simply return error message to the programs.
If the check pass, then the kernel will process these requests.
And now we are in the trusted area where we will not perform the same checks as we did in the barricade.
Instead we will use assertions to find errors within the kernel.
If the assumption we made in the assertions failed, the assertion will stop the system and print the file name and line number which causes the error. So it could help us find some errors as we build up the project