SELECT DISTINCT trainer.name
FROM trainer
JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
JOIN pokemon ON pokemon.id = caughtpokemon.pid
WHERE pokemon.type = 'Psychic'
ORDER BY trainer.name;