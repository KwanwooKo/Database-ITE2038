SELECT DISTINCT trainer.name, city.description
FROM trainer, caughtpokemon, city, pokemon
WHERE pokemon.id = caughtpokemon.pid
AND trainer.id = caughtpokemon.owner_id
AND pokemon.type = 'Fire'
AND trainer.hometown = city.name
ORDER BY trainer.name;