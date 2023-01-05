SELECT COUNT(pokemon)
FROM pokemon, caughtpokemon
WHERE (pokemon.type = 'Water' OR  pokemon.type = 'Electric' OR pokemon.type = 'Psychic')
AND pokemon.id = caughtpokemon.pid;