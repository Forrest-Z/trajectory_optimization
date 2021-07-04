#include <trajectory_optimization/polyTrajGen.h>

polyTraj::polyTraj(){
	this->degree = 6;
	this->velocityd = 1;
	this->diff_degree = 3;
}

polyTraj::polyTraj(double _degree){
	this->degree = _degree;
	this->velocityd = 1;
	this->diff_degree = 3;

}

polyTraj::polyTraj(double _degree, double _velocityd, double _diff_degree){
	this->degree = _degree;
	this->velocityd = _velocityd;
	this->diff_degree = _diff_degree;
}

void polyTraj::loadWaypointPath(const std::vector<pose> &_path){
	this->path = _path;
	this->timed.clear();
	double total_time = 0;
	this->timed.push_back(total_time);
	for (int i=0; i<this->path.size(); ++i){ // calculate desired time for each segment
		if (i != 0){
			double distance = getDistance(this->path[i], this->path[i-1]);
			total_time += distance/velocityd;
			this->timed.push_back(total_time);
		}
	}

	this->constructQp();
	this->constructAb();
	this->constructCd();
}



// Variable Order: [[Cx0, Cxn, Cy0, Cyn.....], [...]]
// Varianle Order [Cx0_1, Cxn_1, Cx0_2], [Cy01 ....]
void polyTraj::constructQp(){
	int num_path_segment = this->path.size() - 1;
	int num_each_coeff = this->degree + 1;
	int dimension = (this->degree+1) * num_path_segment;

	// Construct Q Matrix
	this->Qx.resize(dimension, dimension); this->Qx = 0;
	this->Qy.resize(dimension, dimension); this->Qy = 0;
	this->Qz.resize(dimension, dimension); this->Qz = 0;
	this->Qyaw.resize(dimension, dimension); this->Qyaw = 0;

	// Set p vector (=0)
	this->px.resize(dimension);this->px = 0;
	this->py.resize(dimension);this->py = 0;
	this->pz.resize(dimension);this->pz = 0;
	this->pyaw.resize(dimension);this->pyaw = 0;
	


	for (int n=0; n<num_path_segment; ++n){
		double start_t = this->timed[n]; double end_t =  this->timed[n+1];

		int segment_start_index = n * num_each_coeff;

		// Calculate Hessian Matrix:
		quadprogpp::Matrix<double> f; // create factor matrix for x, y and z seperately
		f.resize(num_each_coeff, 1);
		for (int i=0; i<num_each_coeff; ++i){
			if (i < this->diff_degree){
				f[i][0] = 0;
			}
			else{
				double factor = 1.0;
				for (int j=0; j<this->diff_degree; ++j){
					factor *= (double) (i-j);
				}
				factor *= (double) (1/(i-this->diff_degree+1.0)) * (pow(end_t, i-this->diff_degree+1.0) - pow(start_t, i-this->diff_degree+1.0));
				f[i][0] = factor;
			}
		}

		quadprogpp::Matrix<double> f_yaw; // yaw vector factor
		f_yaw.resize(num_each_coeff, 1);
		for (int i=0; i<num_each_coeff; ++i){
			if (i < 2){
				f_yaw[i][0] = 0;
			}
			else{
				double factor_yaw = (double) i * (i-1.0) * (1.0/(i-1.0)) * (pow(end_t, i-1.0) - pow(start_t, i-1.0));
				f_yaw[i][0] = factor_yaw;
			}
		}


		quadprogpp::Matrix<double> H = dot_prod(f, quadprogpp::t(f)); // Hessian for x, y, z this segment
		quadprogpp::Matrix<double> H_yaw = dot_prod(f_yaw, quadprogpp::t(f_yaw)); // Hessian for yaw this segment

		// cout << "f" << f << endl;
		// cout << "f yaw" << f_yaw << endl;
		// cout << "H" << H << endl;
		// cout << "H yaw" << H_yaw << endl;

		// Assign value to Q:
		for (int row=0; row<num_each_coeff; ++row){
			for (int col=0; col<num_each_coeff; ++col){
				this->Qx[segment_start_index+row][segment_start_index+col] = H[row][col];
				this->Qy[segment_start_index+row][segment_start_index+col] = H[row][col];
				this->Qz[segment_start_index+row][segment_start_index+col] = H[row][col];
				this->Qyaw[segment_start_index+row][segment_start_index+col] = H_yaw[row][col];
			}
		}		
	}

	double perturb = 0.01; // to make them PSD -> PD
	for (int i=0; i<dimension; ++i){
		this->Qx[i][i] += perturb;
		this->Qy[i][i] += perturb;
		this->Qz[i][i] += perturb;
		this->Qyaw[i][i] += perturb;
	}
	// cout << this->Qx << endl;
	// cout << this->Qyaw << endl;
}

void polyTraj::constructAb(){
	int num_waypoint = this->path.size();
	int num_constraint = (2 + (num_waypoint-2)*2) * 4 + 2 * ((num_waypoint - 2) * 4) + (this->diff_degree - 2) * (num_waypoint - 2) * 3 + 2*4;

	int num_constraint_xyz = (2 + (num_waypoint-2)*2) + ((num_waypoint-2)) * this->diff_degree + 2; // zero velocity/acc end xyz
	int num_constraint_yaw =  (2 + (num_waypoint-2)*2) + ((num_waypoint-2)) * 2 + 2; // zero velocity/acc end yaw

	int num_path_segment = this->path.size() - 1;
	int num_each_coeff = this->degree + 1;
	int dimension = (this->degree+1) * num_path_segment;


	this->Ax.resize(num_constraint_xyz, dimension); this->Ax = 0;
	this->Ay.resize(num_constraint_xyz, dimension); this->Ay = 0;
	this->Az.resize(num_constraint_xyz, dimension); this->Az = 0;
	this->Ayaw.resize(num_constraint_yaw, dimension); this->Ayaw = 0; 
	this->bx.resize(num_constraint_xyz); this->bx = 0;
	this->by.resize(num_constraint_xyz); this->by = 0;
	this->bz.resize(num_constraint_xyz); this->bz = 0;
	this->byaw.resize(num_constraint_yaw); this->byaw = 0;

	cout << "expected equality constriants: " << num_constraint << endl;

	int count_constraints = 0; // Total number of constraints
	int count_constraints_xyz = 0;
	int count_constraints_yaw = 0;
	for (int d=0; d<=this->diff_degree; ++d){ // derivative degrees
		if (d == 0){
			for (int i=0; i<num_waypoint; ++i){ // waypoints
				double target_time = this->timed[i];

				for (int s=0; s<2; ++s){ // previous or after segment
					if (i == 0 and s == 0){
						// do nothing: for first waypoint we don't have previous segment
					}
					else if (i == num_waypoint-1 and s == 1){
						// do nothing: for last waypoint we don't have segment after
					}
					else{
						int start_index = (i+s-1) * num_each_coeff;

						for (int c=0; c<num_each_coeff; ++c){// C0 .... Cn
							this->Ax[count_constraints_xyz][start_index+c] = pow(target_time, c);
							this->Ay[count_constraints_xyz][start_index+c] = pow(target_time, c);
							this->Az[count_constraints_xyz][start_index+c] = pow(target_time, c);
							this->Ayaw[count_constraints_yaw][start_index+c] = pow(target_time, c);
						}
						this->bx[count_constraints_xyz] = -this->path[i].x;
						this->by[count_constraints_xyz] = -this->path[i].y;
						this->bz[count_constraints_xyz] = -this->path[i].z;
						this->byaw[count_constraints_yaw] = -this->path[i].yaw;
						count_constraints += 4;
						++count_constraints_xyz;
						++count_constraints_yaw;
						
					}
				}
			}
		}
		else{
			for (int i=0; i<num_waypoint; ++i){
				double target_time = this->timed[i];

				if ((i == 0) or (i == num_waypoint-1)){
					// do nothing: continuity does not apply to end points
				}
				else{
					int start_index1 = (i-1) * num_each_coeff;
					int start_index2 = i * num_each_coeff;

					for (int c=0; c<num_each_coeff; ++c){
						if (c < d){
							this->Ax[count_constraints_xyz][start_index1+c] = 0;
							this->Ax[count_constraints_xyz][start_index2+c] = 0;
							this->Ay[count_constraints_xyz][start_index1+c] = 0;
							this->Ay[count_constraints_xyz][start_index2+c] = 0;
							this->Az[count_constraints_xyz][start_index1+c] = 0;
							this->Az[count_constraints_xyz][start_index2+c] = 0;
						}
						else{
							double factor = 1.0;
							for (int j=0; j<d; j++){
								factor *= (double) (c-j);
							}
							factor *= pow(target_time, c-d);
							this->Ax[count_constraints_xyz][start_index1+c] = factor;
							this->Ax[count_constraints_xyz][start_index2+c] = -factor;
							this->Ay[count_constraints_xyz][start_index1+c] = factor;
							this->Ay[count_constraints_xyz][start_index2+c] = -factor;
							this->Az[count_constraints_xyz][start_index1+c] = factor;
							this->Az[count_constraints_xyz][start_index2+c] = -factor;
							
						}
					}

					++count_constraints_xyz;
					count_constraints += 3;

					if (d <= 2){
						for (int c=0; c<num_each_coeff; ++c){
								if (c < d){
									this->Ayaw[count_constraints_yaw][start_index1+c] = 0;
									this->Ayaw[count_constraints_yaw][start_index2+c] = 0;
								}
								else{
									double factor = 1.0;
									for (int j=0; j<d; j++){
										factor *= (double) (c-j);
									}
									factor *= pow(target_time, c-d);
									this->Ayaw[count_constraints_yaw][start_index1+c] = factor;
									this->Ayaw[count_constraints_yaw][start_index2+c] = -factor;
								}
						}
						++count_constraints_yaw;
						++count_constraints;
					}

				}
			}
		}
	}

	// End constraint (v=0, acceleration=0)
	int zero_diff = 2;
	for (int i=0; i<2; ++i){// each end points
		int end_index;
		double target_time;

		if (i == 0){
			end_index = 0;
			target_time = this->timed[0];
		}
		else if (i == 1){
			end_index = (num_path_segment-1) * num_each_coeff;
			target_time = this->timed[num_waypoint-1];
		}

		for (int d=1; d<zero_diff; ++d){
			for (int c=0; c<num_each_coeff; ++c){
				if (c < d){
					this->Ax[count_constraints_xyz][end_index+c] = 0;
					this->Ay[count_constraints_xyz][end_index+c] = 0;
					this->Az[count_constraints_xyz][end_index+c] = 0;
					this->Ayaw[count_constraints_yaw][end_index+c] = 0;
				}
				else{
					double factor = 1.0;
					for (int j=0; j<d; j++){
						factor *= (double) (c-j);
					}
					factor *= pow(target_time, c-d);
					this->Ax[count_constraints_xyz][end_index+c] = factor;
					this->Ay[count_constraints_xyz][end_index+c] = factor;
					this->Az[count_constraints_xyz][end_index+c] = factor;
					this->Ayaw[count_constraints_yaw][end_index+c] = factor;
				}

			}
		}

		++count_constraints_xyz;
		++count_constraints_yaw;
		count_constraints += 4;
	}	

	cout << "number of equality constraints: " << count_constraints << endl;
}

void polyTraj::constructCd(){
	// C d is zero
	int num_path_segment = this->path.size() - 1;
	int dimension = (this->degree+1) * num_path_segment;
	this->Cx.resize(1, dimension); this->Cx = 0;
	this->Cy.resize(1, dimension); this->Cy = 0;
	this->Cz.resize(1, dimension); this->Cz = 0;
	this->Cyaw.resize(1, dimension); this->Cyaw = 0;

	this->dx.resize(1);this->dx = 0;
	this->dy.resize(1);this->dy = 0;
	this->dz.resize(1);this->dz = 0;
	this->dyaw.resize(1);this->dyaw = 0;
	
}

void polyTraj::optimize(){
	int num_path_segment = this->path.size() - 1;
	int dimension = ((this->degree+1) * 4) * num_path_segment;

	quadprogpp::Vector<double> x_param, y_param, z_param, yaw_param;
	double min_result_x = solve_quadprog(this->Qx, this->px, quadprogpp::t(this->Ax), this->bx, quadprogpp::t(this->Cx), this->dx, x_param);
	double min_result_y = solve_quadprog(this->Qy, this->py, quadprogpp::t(this->Ay), this->by, quadprogpp::t(this->Cy), this->dy, y_param);
	double min_result_z = solve_quadprog(this->Qz, this->pz, quadprogpp::t(this->Az), this->bz, quadprogpp::t(this->Cz), this->dz, z_param);
	double min_result_yaw = solve_quadprog(this->Qyaw, this->pyaw, quadprogpp::t(this->Ayaw), this->byaw, quadprogpp::t(this->Cyaw), this->dyaw, yaw_param);

	this->x_param_sol = x_param;
	this->y_param_sol = y_param;
	this->z_param_sol = z_param;
	this->yaw_param_sol = yaw_param;

	cout << x_param << endl;

}

pose polyTraj::getPose(double t){
	int num_each_coeff = this->degree + 1;
	
	pose p;
	for (int i=0; i<this->timed.size()-1; ++i){
		double start_t = this->timed[i];
		double end_t = this->timed[i+1];
		if ((t >= start_t) and (t <= end_t)){
			int coeff_start_index = i*num_each_coeff;
			double x = 0; double y = 0; double z = 0; double yaw = 0;
			for (int n=0; n<num_each_coeff; ++n){
				x += this->x_param_sol[coeff_start_index+n] * pow(t, n);
				y += this->y_param_sol[coeff_start_index+n] * pow(t, n);
				z += this->z_param_sol[coeff_start_index+n] * pow(t, n);
				yaw += this->yaw_param_sol[coeff_start_index+n] * pow(t, n);
			}

			p.x = x; p.y = y; p.z = z; p.yaw = yaw;
		}
	}
	return p;
}

std::vector<pose> polyTraj::getTrajectory(double delT){
	this->trajectory.clear();
	double t_final = this->timed[this->timed.size() - 1];
	for (double t=0; t < t_final; t+=delT){
		pose p = this->getPose(t);
		this->trajectory.push_back(p);
	}
	return this->trajectory;
}

void polyTraj::printWaypointPath(){
	cout << fixed << setprecision(2); 
	if (this->path.size() == 0){
		cout << "You have to load path first!" << endl;
	}
	else{
		int count = 0;
		for (pose p: this->path){
			if (count != 0){
				// double distance = sqrt(pow((p.x - this->path[count-1].x),2) + pow((p.y - this->path[count-1].y),2) + pow((p.z - this->path[count-1].z),2));	
				double distance = getDistance(p, path[count-1]);
				cout << ">>[Gap (" << count-1 << "->" << count << "): " << distance << "m.]<<"<< endl;			
			}
			cout << "Waypoint " << count << ": (" << p.x << ", " << p.y << ", " << p.z << "), yaw: " << p.yaw << ". |TIME: " << this->timed[count] << " s.|" <<endl;
			++count; 
		}
	}
}

void polyTraj::printTrajectory(){
	cout << fixed << setprecision(2); 
	if (this->trajectory.size() == 0){
		cout << "You have to optimize first!" << endl;
	}
	else{
		int count = 0;
		for (pose p: this->trajectory){
			cout << "Pose " << count << ": (" << p.x << ", " << p.y << ", " << p.z << "), yaw: " << p.yaw << ". "<< endl;
			++count; 
		}
	}
}


