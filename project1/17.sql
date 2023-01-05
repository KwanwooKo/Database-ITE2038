SELECT AVG(caughtpokemon.level)
FROM caughtpokemon, pokemon
WHERE pokemon.type = 'Water'
AND caughtpokemon.pid = pokemon.id;