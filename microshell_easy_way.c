/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell_easy_way.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: del-yaag <del-yaag@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/13 13:51:23 by del-yaag          #+#    #+#             */
/*   Updated: 2023/09/20 13:51:55 by del-yaag         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int printErr( char *str ) {

	while ( *str )
		write( 2, str++, 1 );
	return 1;
}

int cd( char **argv, int i ) {

	if ( i != 2 )
		return printErr( "error: cd: bad arguments\n" );
	else if ( chdir( argv[1] ) )
		return printErr( "error: cd: cannot change directory to " ), printErr( argv[i] ), printErr( "\n" );
	return 0;
}

int execution( char **argv, char **env, int i ) {

	int fd[2];
	int pid;
	int status;
	int ispipe;

	ispipe = argv[i] && !strcmp( argv[i], "|" );
	if ( ispipe && pipe( fd ) == -1 )
		return printErr( "error: fatal\n" );
	pid = fork();
	if ( pid == -1 )
		return printErr( "error: fatal\n" );
	if ( !pid ) {

		argv[i] = 0;
		if ( ispipe && ( dup2( fd[1], 1 ) == -1 || close( fd[1] ) == -1 || close( fd[0] ) == -1 ) )
			return printErr( "error: fatal\n" );
		execve( *argv, argv, env );
		return printErr("error: cannot execute " ), printErr( *argv ), printErr( "\n" );
	}
	waitpid( pid, &status, 0 );
	if ( ispipe && (dup2( fd[0], 0 ) == -1 || close( fd[1] ) == -1 || close( fd[0] ) == -1 ) )
		return printErr( "error: fatal\n" );
	return WIFEXITED( status ) && WEXITSTATUS( status );
}

int main( int argc, char ** argv, char **env ) {

	int i;
	int status;

	i = 0;
	status = 0;
	if ( argc > 1 ) {

		while ( argv[i] && argv[++i] ) {

			argv += i;
			i = 0;
			while ( argv[i] && strcmp( argv[i], "|" ) && strcmp( argv[i], ";" ) )
				i++;
			if ( !strcmp( *argv, "cd" ) )
				status = cd( argv, i );
			else if ( i )
				status = execution( argv, env, i );
		}
	}
	return status;
}