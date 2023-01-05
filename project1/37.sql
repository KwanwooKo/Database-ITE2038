SELECT DISTINCT caughtpokemon.pid, caughtpokemon.nickname
FROM pokemon
JOIN caughtpokemon ON caughtpokemon.pid = pokemon.id
JOIN trainer ON trainer.id = caughtpokemon.owner_id
WHERE pokemon.type = 'Water'
AND trainer.hometown = 'Blue City'
ORDER BY caughtpokemon.pid, caughtpokemon.nickname;