SELECT DISTINCT pokemon.name, caughtpokemon.level
FROM trainer
JOIN gym ON gym.leader_id = trainer.id
JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
JOIN pokemon ON pokemon.id = caughtpokemon.pid
WHERE gym.city = 'Sangnok City'
ORDER BY caughtpokemon.level, pokemon.name;