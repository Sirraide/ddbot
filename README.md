To build this you need the [dpp library](https://github.com/Sirraide/DPP), as well as `cmake`.

Also, you need to create a file called `token.h` in the `src` folder and `#define` `TOKEN` as a string literal containing
your bot token, as well as add a variable of type `dpp::snowflake` called `server_id` that contains the id of your 
discord server.