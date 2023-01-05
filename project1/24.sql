SELECT pokemon.name
FROM caughtpokemon, trainer, pokemon
WHERE caughtpokemon.owner_id = trainer.id
AND pokemon.id = caughtpokemon.pid
AND (trainer.hometown = 'Sangnok City'
        OR trainer.hometown = 'Brown City')
GROUP BY pokemon.name
ORDER BY pokemon.name;