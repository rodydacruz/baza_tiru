extends Node2D


var is_game_over = false
var events = 0
var asteroid = preload("res://scenes/asteroid.tscn")
var dark = preload("res://scenes/dark.tscn")
var low_nemy = preload("res://scenes/low_nemy.tscn")

const SCREEN_WIDTH = 320
const SCREEN_HEIGHT = 180
var score = 0

func _ready():
	get_node("player").connect("destroyed", Callable(self, "_on_player_destroyed"))
	get_node("spawn_timer").connect("timeout", Callable(self, "_on_spawn_timer_timeout"))
	
func _input(event):
	if Input.is_key_pressed(KEY_ESCAPE):
		get_tree().quit()
	if is_game_over and Input.is_key_pressed(KEY_ENTER):
		get_tree().change_scene_to_file("res://scenes/stage.tscn")
		
		
func _on_player_destroyed():
	get_node("ui/retry").show()
	is_game_over = true


func _on_spawn_timer_timeout():
	var asteroid_instance = asteroid.instantiate()
	asteroid_instance.position = Vector2(SCREEN_WIDTH + 8, randf_range(0,SCREEN_HEIGHT))
	asteroid_instance.connect("score", Callable(self, "_on_player_score"))
	add_child(asteroid_instance)
	
	if randi() % 5 == 0:
		var enemy_instance = low_nemy.instantiate()
		enemy_instance.position = Vector2(SCREEN_WIDTH + 16,randf_range(20,SCREEN_HEIGHT - 20))
		enemy_instance.connect("score", Callable(self, "_on_player_score"))
		
		add_child(enemy_instance)
	
	
func _on_player_score(points, enemy_id):
	score += points
	get_node("ui/score").text = "Score: " + str(score)
	print("Enemy defeated with ID:", enemy_id, " +", points, "points")
	_runScorePhase(score)
	
func _runScorePhase(score):
	if events <= 0:
		if 20 < score < 100:
			# Creating a Field Of Darks
			var lastPosition
			for d in [1,2,3,4,5,6,7,8,9,10,11,12]:
				var dark_instance = dark.instantiate()
				var position = randf_range(SCREEN_WIDTH + 8,2 * SCREEN_WIDTH)
				events += 1
				dark_instance.position = Vector2(position, randf_range(0,SCREEN_HEIGHT))
				dark_instance.connect("score", Callable(self, "_on_player_score"))
				
				add_child(dark_instance)
		if 100 < score < 200:
			# Lunch BossNemy01 
			print("Reched Score",score)
